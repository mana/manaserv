/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */


#include <sstream>

#include "dalstorage.h"
#include "dalstoragesql.h"


namespace
{


/**
 * Functor used for the search of an Account by name.
 */
struct account_name_equals_to
    : public std::binary_function<Account*, std::string, bool>
{
    bool
    operator()(Account* account,
               const std::string& name) const
    {
        return account->getName() == name;
    }
};


/**
 * Functor to convert a string into another type using
 * std::istringstream.operator>>().
 */
template <typename T>
struct string_to: public std::unary_function<std::string, T>
{
    T
    operator()(const std::string& s) const
    {
        std::istringstream is(s);
        T value;
        is >> value;

        return value;
    }
};


} // anonymous namespace


namespace tmwserv
{


/**
 * Constructor.
 */
DALStorage::DALStorage()
        : mDb(dal::DataProviderFactory::createDataProvider())
{
    // the connection to the database will be made on the first request
    // to the database.
}


/**
 * Destructor.
 */
DALStorage::~DALStorage()
    throw()
{
    mDb->disconnect();

    // clean up loaded accounts.
    for (Accounts::iterator it = mAccounts.begin();
         it != mAccounts.end();
         ++it)
    {
        delete (*it);
    }

    // clean up loaded characters.
    for (Beings::iterator it = mCharacters.begin();
         it != mCharacters.end();
         ++it)
    {
        delete (*it);
    }
}


/**
 * Save changes to the database permanently.
 */
void
DALStorage::flush(void)
{
    // this feature is not currently provided by DAL.
}


/**
 * Get the number of Accounts saved in database.
 */
unsigned int
DALStorage::getAccountCount(void)
{
    // connect to the database (if not connected yet).
    connect();

    using namespace dal;

    try {
        // query the database.
        std::string sql("select count(*) from ");
        sql += ACCOUNTS_TBL_NAME;
        sql += ";";
        const RecordSet& rs = mDb->execSql(sql);

        // specialize the string_to functor to convert
        // a string to an unsigned int.
        string_to<unsigned int> toUint;

        return toUint(rs(0, 0));
    } catch (const DbSqlQueryExecFailure& f) {
        std::cout << "Get accounts count failed :'(" << std::endl;
    }
}


/**
 * Get an account by user name.
 */
Account*
DALStorage::getAccount(const std::string& userName)
{
    // connect to the database (if not connected yet).
    connect();

    // look for the account in the list first.
    Accounts::iterator it =
        std::find_if(
            mAccounts.begin(),
            mAccounts.end(),
            std::bind2nd(account_name_equals_to(), userName)
        );

    if (it != mAccounts.end()) {
        return (*it);
    }

    using namespace dal;

    // the account was not in the list, look for it in the database.
    try {
        std::string sql("select * from ");
        sql += ACCOUNTS_TBL_NAME;
        sql + " where username = '";
        sql += userName;
        sql += "';";
        const RecordSet& accountInfo = mDb->execSql(sql);

        // if the account is not even in the database then
        // we have no choice but to return nothing.
        if (accountInfo.isEmpty()) {
            return NULL;
        }

        // create an Account instance
        // and initialize it with information about the user.
        Account* account = new Account();
        account->setName(accountInfo(0, 1));
        account->setPassword(accountInfo(0, 2));
        account->setEmail(accountInfo(0, 3));

        // add the new Account to the list.
        mAccounts.push_back(account);

        // load the characters associated with the account.
        sql = "select * from ";
        sql += CHARACTERS_TBL_NAME;
        sql += " where id = '";
        sql += accountInfo(0, 0);
        sql += "';";
        const RecordSet& charInfo = mDb->execSql(sql);

        if (!charInfo.isEmpty()) {
            Beings beings;

            // specialize the string_to functor to convert
            // a string to an unsigned int.
            string_to<unsigned int> toUint;

            for (unsigned int i = 0; i < charInfo.rows(); ++i) {
                Being* being =
                    new Being(charInfo(i, 2),          // name
                              toUint(charInfo(i, 3)),  // gender
                              toUint(charInfo(i, 4)),  // level
                              toUint(charInfo(i, 5)),  // money
                              toUint(charInfo(i, 9)),  // strength
                              toUint(charInfo(i, 10)), // agility
                              toUint(charInfo(i, 11)), // vitality
                              toUint(charInfo(i, 13)), // dexterity
                              toUint(charInfo(i, 14))  // luck
                             );

                mCharacters.push_back(being);
                beings.push_back(being);
            }

            account->setCharacters(beings);
        }

        return account;
    }
    catch (const DbSqlQueryExecFailure& e) {
        return NULL; // TODO: Throw exception here
    }
}


/**
 * Connect to the database and initialize it if necessary.
 */
void
DALStorage::connect(void)
{
    // do nothing if already connected.
    if (mDb->isConnected()) {
        return;
    }

    using namespace dal;

    try {
        // open a connection to the database.
        // TODO: get the database name, the user name and the user password
        // from a configuration manager.
        mDb->connect("tmw", "", "");

        // ensure that the required tables are created.
        //
        // strategy1: find a way to obtain the list of tables from the
        //            underlying database and create the tables that are
        //            missing.
        //
        // strategy2: try to create the tables and check the exceptions
        //            thrown.
        //
        // comments:
        //     - strategy1 is easy to achieve if we are using MysQL as
        //       executing the request "show tables;" returns the list of
        //       tables. However, there is not such a query for SQLite3.
        //       When using SQLite3 from the interactive shell or the
        //       command line, the command ".tables" returns the list of
        //       tables but sqlite3_exec() does not validate this statement
        //       and fails.
        //       The cost of this strategy is:
        //           (num. tables to create + 1) queries at most and
        //           1 at minimum.
        //
        //     - strategy2 will work with probably most databases.
        //       The cost of this strategy is:
        //           (num. tables to create) queries.

        // we will stick with strategy2 for the moment as we are focusing
        // on SQLite.

        createTable(MAPS_TBL_NAME, SQL_MAPS_TABLE);
        createTable(ACCOUNTS_TBL_NAME, SQL_ACCOUNTS_TABLE);
        createTable(CHARACTERS_TBL_NAME, SQL_CHARACTERS_TABLE);
        createTable(ITEMS_TBL_NAME, SQL_ITEMS_TABLE);
        createTable(WORLD_ITEMS_TBL_NAME, SQL_WORLD_ITEMS_TABLE);
        createTable(INVENTORIES_TBL_NAME, SQL_INVENTORIES_TABLE);

        // Example data :)
        mDb->execSql("insert into tmw_accounts values (0, 'nym', 'tHiSiSHaShEd', 'nym@test', 1, 0);");
        mDb->execSql("insert into tmw_accounts values (1, 'Bjorn', 'tHiSiSHaShEd', 'bjorn@test', 1, 0);");
        mDb->execSql("insert into tmw_accounts values (2, 'Usiu', 'tHiSiSHaShEd', 'usiu@test', 1, 0);");
        mDb->execSql("insert into tmw_accounts values (3, 'ElvenProgrammer', 'tHiSiSHaShEd', 'elven@test', 1, 0);");
        mDb->execSql("insert into tmw_characters values (0, 0, 'Nym the Great', 0, 99, 1000000, 0, 0, 'main.map', 1, 2, 3, 4, 5, 6);");
    }
    catch (const DbConnectionFailure& e) {
        std::cout << "unable to connect to the database: "
                  << e.what() << std::endl;
    }
    catch (const DbSqlQueryExecFailure& e) {
        std::cout << e.what() << std::endl;
    }
}


/**
 * Create the specified table.
 */
void
DALStorage::createTable(const std::string& tblName,
                        const std::string& sql)
{
    try {
        mDb->execSql(sql);
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // error message to check against.
        std::string alreadyExists("table ");
        alreadyExists += tblName;
        alreadyExists += " already exists";

        const std::string msg(e.what());

        // oops, another problem occurred.
        if (msg != alreadyExists) {
            // rethrow to let other error handlers manage the problem.
            throw;
        }
    }
}


} // namespace tmwserv
