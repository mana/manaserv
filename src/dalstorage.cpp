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
#include <vector>

#include "utils/cipher.h"
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
#if defined (MYSQL_SUPPORT) || defined (POSTGRESQL_SUPPORT)
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
        Account* account = new Account(accountInfo(0, 1),
                                       accountInfo(0, 2),
                                       accountInfo(0, 3));

        // specialize the string_to functor to convert
        // a string to an unsigned int.
        string_to<unsigned short> toUint;

        // specialize the string_to functor to convert
        // a string to an unsigned short.
        string_to<unsigned short> toUshort;

        // add the new Account to the list.
        AccountInfo ai;
        ai.status = AS_ACC_TO_UPDATE;
        ai.id = toUint(accountInfo(0, 0));
        mAccounts.insert(std::make_pair(account, ai));

        // load the characters associated with the account.
        sql = "select * from ";
        sql += CHARACTERS_TBL_NAME;
        sql += " where id = '";
        sql += accountInfo(0, 0);
        sql += "';";
        const RecordSet& charInfo = mDb->execSql(sql);

        if (!charInfo.isEmpty()) {
            Beings beings;

            for (unsigned int i = 0; i < charInfo.rows(); ++i) {
                RawStatistics stats = {
                    toUshort(charInfo(i, 9)),  // strength
                    toUshort(charInfo(i, 10)), // agility
                    toUshort(charInfo(i, 11)), // vitality
                    toUshort(charInfo(i, 12)), // intelligence
                    toUshort(charInfo(i, 13)), // dexterity
                    toUshort(charInfo(i, 14))  // luck
                };

                // convert the integer value read from database
                // to an enum because C++ does not allow implicit
                // conversion from an integer to an enum.
                unsigned short value = toUshort(charInfo(i, 3));
                Genders gender;
                switch (value)
                {
                    case 0:
                        gender = GENDER_MALE;
                        break;

                    case 1:
                        gender = GENDER_FEMALE;
                        break;

                    default:
                        gender = GENDER_UNKNOWN;
                        break;
                };

                Being* being =
                    new Being(charInfo(i, 2),           // name
                              gender,                   // gender
                              toUshort(charInfo(i, 4)), // level
                              toUint(charInfo(i, 5)),   // money
                              stats
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
DALStorage::addAccount(const Account* account)
{
    if (account == 0) {
        // maybe we should throw an exception instead
        return;
    }

    // mark this account as new so that the next flush will execute a SQL
    // insert query instead of a SQL update query.
    AccountInfo ai;
    ai.status = AS_NEW_ACCOUNT;
    // the account id is set to 0 because we know nothing about it at the
    // moment, it will be updated once saved into the database.
    ai.id = 0;
    mAccounts.insert(std::make_pair(const_cast<Account*>(account), ai));
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
        switch ((it->second).status) {
            case AS_NEW_ACCOUNT:
                {
                    // this is a newly added account and it has not even been
                    // saved into the database: remove it immediately.

                    // TODO: delete the associated characters.

                    delete it->first;

                    // TODO: remove from the map.
                }
                break;

            case AS_ACC_TO_UPDATE:
                // change the status to AS_ACC_TO_DELETE so that it will be
                // deleted at the next flush.
                (it->second).status = AS_ACC_TO_DELETE;
                break;

            default:
                break;
        }

        // nothing else to do.
        return;
    }

    using namespace dal;

    try {
        // look for the account directly into the database.
        _delAccount(userName);
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
    }
}


/**
 * Save changes to the database permanently.
 */
void
DALStorage::flush(void)
{
    Accounts::const_iterator it = mAccounts.begin();
    Accounts::const_iterator it_end = mAccounts.end();
    for (; it != it_end; ++it) {
        switch ((it->second).status) {
            case AS_NEW_ACCOUNT:
                _addAccount(it->first);
                break;

            case AS_ACC_TO_UPDATE:
                _updAccount(it->first);
                break;

            case AS_ACC_TO_DELETE:
                // TODO: accounts to be deleted must be handled differently
                // as mAccounts will be altered once the accounts are deleted.
                break;

            default:
                break;
        }
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
#if defined (MYSQL_SUPPORT)
        std::string alreadyExists("Table '");
        alreadyExists += tblName;
        alreadyExists += "' already exists";
#elif defined (POSTGRESQL_SUPPORT)
        // TODO
#else // SQLITE_SUPPORT
        std::string alreadyExists("table ");
        alreadyExists += tblName;
        alreadyExists += " already exists";
#endif

        const std::string msg(e.what());

        // oops, another problem occurred.
        if (msg != alreadyExists) {
            // rethrow to let other error handlers manage the problem.
            throw;
        }
    }
}


/**
 * Add an account to the database.
 */
void
DALStorage::_addAccount(const Account* account)
{
    if (account == 0) {
        return;
    }

    // assume that account is an element of mAccounts as this method is
    // private and only called by flush().

    using namespace dal;

    // TODO: we should start a transaction here so that in case of problem
    // the lost of data would be minimized.

    // insert the account.
    std::ostringstream sql1;
    sql1 << "insert into " << ACCOUNTS_TBL_NAME << " values (null, '"
         << account->getName() << "', '"
         << account->getPassword() << "', '"
         << account->getEmail() << "', "
         << account->getLevel() << ", 0);";
    mDb->execSql(sql1.str());

    // get the account id.
    std::ostringstream sql2;
    sql2 << "select id from " << ACCOUNTS_TBL_NAME
         << " where username = '" << account->getName() << "';";
    const RecordSet& accountInfo = mDb->execSql(sql2.str());
    string_to<unsigned int> toUint;

    Accounts::iterator account_it =
        std::find_if(
            mAccounts.begin(),
            mAccounts.end(),
            account_by_name(account->getName())
        );

    // update the info of the account.
    (account_it->second).status = AS_ACC_TO_UPDATE;
    (account_it->second).id = toUint(accountInfo(0, 0));

    // insert the characters.
    Beings& characters = (const_cast<Account*>(account))->getCharacters();

    Beings::const_iterator it = characters.begin();
    Beings::const_iterator it_end = characters.end();
    for (; it != it_end; ++it) {
        RawStatistics& stats = (*it)->getRawStatistics();
        std::ostringstream sql3;
        sql3 << "insert into " << CHARACTERS_TBL_NAME << " values (null, "
             << (account_it->second).id << ", '"
             << (*it)->getName() << "', '"
             << (*it)->getGender() << "', "
             << (*it)->getLevel() << ", "
             << (*it)->getMoney() << ", "
             << (*it)->getX() << ", "
             << (*it)->getY() << ", "
             << "0, " // TODO: map id
             << stats.strength << ", "
             << stats.agility << ", "
             << stats.vitality << ", "
             << stats.intelligence << ", "
             << stats.dexterity << ", "
             << stats.luck
             << ");";
        mDb->execSql(sql3.str());

        // TODO: inventories.
    }
}


/**
 * Update an account from the database.
 */
void
DALStorage::_updAccount(const Account* account)
{
    if (account == 0) {
        return;
    }

    // assume that account is an element of mAccounts as this method is
    // private and only called by flush().

    using namespace dal;

    // TODO: we should start a transaction here so that in case of problem
    // the lost of data would be minimized.

    Accounts::iterator account_it =
        std::find_if(
            mAccounts.begin(),
            mAccounts.end(),
            account_by_name(account->getName())
        );

    // doublecheck that this account already exists in the database
    // and therefore its status must be AS_ACC_TO_UPDATE.
    if ((account_it->second).status != AS_ACC_TO_UPDATE) {
        return; // should we throw an exception here instead?
    }

    // update the account.
    std::ostringstream sql1;
    sql1 << "update " << ACCOUNTS_TBL_NAME
         << " set username = '" << account->getName() << "', "
         << "password = '" << account->getPassword() << "', "
         << "email = '" << account->getEmail() << "', "
         << "level = '" << account->getLevel() << "' "
         << "where id = '" << (account_it->second).id << "';";
    mDb->execSql(sql1.str());

    // get the list of characters that belong to this account.
    Beings& characters = (const_cast<Account*>(account))->getCharacters();

    // insert or update the characters.
    Beings::const_iterator it = characters.begin();
    Beings::const_iterator it_end = characters.end();
    using namespace dal;

    for (; it != it_end; ++it) {
        // check if the character already exists in the database
        // (reminder: the character names are unique in the database).
        std::ostringstream sql2;
        sql2 << "select id from " << CHARACTERS_TBL_NAME
             << " where name = '" << (*it)->getName() << "';";
        const RecordSet& charInfo = mDb->execSql(sql2.str());

        RawStatistics& stats = (*it)->getRawStatistics();

        std::ostringstream sql3;
        if (charInfo.rows() == 0) {
            sql3 << "insert into " << CHARACTERS_TBL_NAME
                 << " values (null, "
                 << (account_it->second).id << ", '"
                 << (*it)->getName() << "', "
                 << (*it)->getGender() << ", "
                 << (*it)->getLevel() << ", "
                 << (*it)->getMoney() << ", "
                 << (*it)->getX() << ", "
                 << (*it)->getY() << ", "
                 << "0, " // TODO: map id
                 << stats.strength << ", "
                 << stats.agility << ", "
                 << stats.vitality << ", "
                 << stats.intelligence << ", "
                 << stats.dexterity << ", "
                 << stats.luck << ");";
        }
        else {
            sql3 << "update " << CHARACTERS_TBL_NAME
                << " set name = '" << (*it)->getName() << "', "
                << " gender = " << (*it)->getGender() << ", "
                << " level = " << (*it)->getLevel() << ", "
                << " money = " << (*it)->getMoney() << ", "
                << " x = " << (*it)->getX() << ", "
                << " y = " << (*it)->getY() << ", "
                << " map_id = 0, " // TODO: map id
                << " str = " << stats.strength << ", "
                << " agi = " << stats.agility << ", "
                << " vit = " << stats.vitality << ", "
#ifdef MYSQL_SUPPORT
                << " `int` = " << stats.intelligence << ", "
#else
                << " int = " << stats.intelligence << ", "
#endif
                << " dex = " << stats.dexterity << ", "
                << " luck = " << stats.luck
                << " where id = " << charInfo(0, 0) << ";";
        }
        mDb->execSql(sql3.str());

        // TODO: inventories.
    }
}


/**
 * Delete an account and its associated data from the database.
 */
void
DALStorage::_delAccount(const std::string& userName)
{
    using namespace dal;

    // TODO: optimize, we may be doing too much SQL queries here but this
    // code should work with any database :(

    // get the account id.
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

    // save the account id.
    std::string accountId(accountInfo(0, 0));

    // get the characters that belong to the account.
    sql = "select id from ";
    sql += CHARACTERS_TBL_NAME;
    sql += " where user_id = '";
    sql += accountId;
    sql += "';";
    const RecordSet& charsInfo = mDb->execSql(sql);

    // save the character ids.
    using namespace std;
    vector<string> charIds;
    for (unsigned int i = 0; i < charsInfo.rows(); ++i) {
        charIds.push_back(charsInfo(i, 0));
    }

    // TODO: we should start a transaction here so that in case of problem
    // the lost of data would be minimized.

    // actually removing data.
    vector<string>::const_iterator it = charIds.begin();
    vector<string>::const_iterator it_end = charIds.end();
    for (; it != it_end; ++it) {
        // delete the inventory.
        sql = "delete from ";
        sql += INVENTORIES_TBL_NAME;
        sql += " where owner_id = '";
        sql += (*it);
        sql += "';";
        mDb->execSql(sql);

        // now delete the character.
        sql = "delete from ";
        sql += CHARACTERS_TBL_NAME;
        sql += " where id = '";
        sql += (*it);
        sql += "';";
        mDb->execSql(sql);
    }

    // delete the account.
    sql = "delete from ";
    sql += ACCOUNTS_TBL_NAME;
    sql += " where id = '";
    sql += accountId;
    sql += "';";
    mDb->execSql(sql);
}


} // namespace tmwserv
