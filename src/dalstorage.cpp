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

#include "utils/functors.h"
#include "utils/logger.h"

#include "dalstorage.h"
#include "dalstoragesql.h"


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
    if (mDb->isConnected()) {
        close();
    }

    // clean up accounts.
    for (Accounts::iterator it = mAccounts.begin();
         it != mAccounts.end();
         ++it)
    {
        delete it->first;
    }

    // clean up characters.
    for (Beings::iterator it = mCharacters.begin();
         it != mCharacters.end();
         ++it)
    {
        delete (*it);
    }
}


/**
 * Connect to the database and initialize it if necessary.
 */
void
DALStorage::open(void)
{
    // do nothing if already connected.
    if (mDb->isConnected()) {
        return;
    }

    using namespace dal;

    try {
        // open a connection to the database.
#if defined (MYSQL_SUPPORT) || defined (POSTGRE_SUPPORT)
        mDb->connect(getName(), getUser(), getPassword());
#else // SQLITE_SUPPORT
        // create the database file name.
        std::string dbFile(getName());
        dbFile += ".db";
        mDb->connect(dbFile, "", "");
#endif

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
    }
    catch (const DbConnectionFailure& e) {
        LOG_ERROR("unable to connect to the database: " << e.what())
    }
    catch (const DbSqlQueryExecFailure& e) {
        LOG_ERROR("SQL query failure: " << e.what())
    }

    mIsOpen = mDb->isConnected();
}


/**
 * Disconnect from the database.
 */
void
DALStorage::close(void)
{
    mDb->disconnect();
    mIsOpen = mDb->isConnected();
}


/**
 * Get an account by user name.
 */
Account*
DALStorage::getAccount(const std::string& userName)
{
    // connect to the database (if not connected yet).
    open();

    // look for the account in the list first.
    Accounts::iterator it =
        std::find_if(
            mAccounts.begin(),
            mAccounts.end(),
            account_by_name(userName)
        );

    if (it != mAccounts.end()) {
        return it->first;
    }

    using namespace dal;

    // the account was not in the list, look for it in the database.
    try {
        std::string sql("select * from ");
        sql += ACCOUNTS_TBL_NAME;
        sql += " where username = '";
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
        mAccounts.insert(std::make_pair(account, AS_ACC_TO_UPDATE));

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
 * Add a new account.
 */
void
DALStorage::addAccount(Account* account)
{
    if (account == 0) {
        // maybe we should throw an exception instead
        return;
    }

    // mark this account as new so that the next flush will execute a SQL
    // insert query instead of a SQL update query.
    mAccounts.insert(std::make_pair(account, AS_NEW_ACCOUNT));
}


/**
 * Delete an account.
 */
void
DALStorage::delAccount(const std::string& userName)
{
    // look for the account in memory first.
    Accounts::iterator it =
        std::find_if(
            mAccounts.begin(),
            mAccounts.end(),
            account_by_name(userName)
        );

    if (it != mAccounts.end()) {
        switch (it->second) {
            case AS_NEW_ACCOUNT:
                // this is a newly added account and it has not even been
                // saved into the database: remove it immediately.
                delete it->first;
                break;

            case AS_ACC_TO_UPDATE:
                // change the status to AS_ACC_TO_DELETE so that it will be
                // deleted at the next flush.
                it->second = AS_ACC_TO_DELETE;
                break;

            default:
                break;
        }

        // nothing else to do.
        return;
    }

    using namespace dal;

    try {
        std::string sql("select id from ");
        sql += ACCOUNTS_TBL_NAME;
        sql += " where username = '";
        sql += userName;
        sql += "';";
        const RecordSet& accountInfo = mDb->execSql(sql);

        // the account does not even exist in the database,
        // there is nothing to do then.
        if (accountInfo.isEmpty()) {
            return;
        }

        // TODO: actually deleting the account from the database.
        // order of deletion:
        //     1. inventories of all the characters of the account,
        //     2. all the characters,
        //     3. the account itself.
    }
    catch (const DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
    }
}


/**
 * Save changes to the database permanently.
 */
void
DALStorage::flush(void)
{
    // TODO
    // For each account in memory:
    //     - get the status
    //     - if AS_NEW_ACCOUNT then insert into database;
    //     - if AS_ACC_TO_UPDATE then update values from the database
    //     - if AS_ACC_TO_DELETE then delete from database
    // Notes:
    //     - this will probably involve more than one table as the account may
    //       have characters associated to it.
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
