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

#include "dalstorage.h"

#include "configuration.h"
#include "dalstoragesql.h"

#include "dal/dalexcept.h"
#include "dal/dataproviderfactory.h"

#include "utils/cipher.h"
#include "utils/functors.h"
#include "utils/logger.h"

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

    // mAccounts and mCharacters contain smart pointers that will deallocate
    // the memory so nothing else to do here :)
}


/**
 * Connect to the database and initialize it if necessary.
 */
void
DALStorage::open(void)
{
    // Do nothing if already connected.
    if (mDb->isConnected()) {
        return;
    }

    using namespace dal;

    static bool dbFileShown = false;
    std::string dbFile(getName());
    try {
        // open a connection to the database.
#if defined (MYSQL_SUPPORT) || defined (POSTGRESQL_SUPPORT)
        mDb->connect(getName(), getUser(), getPassword());
        if (!dbFileShown)
        {
            LOG_INFO("Using " << dbFile << " as Database Name.", 0);
            dbFileShown = true;
        }
#elif defined (SQLITE_SUPPORT)
        // create the database file name.
        dbFile += ".db";
        mDb->connect(dbFile, "", "");
        if (!dbFileShown)
        {
            LOG_INFO("SQLite uses ./" << dbFile << " as DB.", 0);
            dbFileShown = true;
        }
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

        // FIXME: The tables should be checked/created at startup in order to avoid
        // a DbSqlQueryExecFailure assert on sqlite while registering.
        // Also, this would initialize connection to the database earlier in memory.

        createTable(MAPS_TBL_NAME, SQL_MAPS_TABLE);
        createTable(ACCOUNTS_TBL_NAME, SQL_ACCOUNTS_TABLE);
        createTable(CHARACTERS_TBL_NAME, SQL_CHARACTERS_TABLE);
        createTable(ITEMS_TBL_NAME, SQL_ITEMS_TABLE);
        createTable(WORLD_ITEMS_TBL_NAME, SQL_WORLD_ITEMS_TABLE);
        createTable(INVENTORIES_TBL_NAME, SQL_INVENTORIES_TABLE);
        createTable(CHANNELS_TBL_NAME, SQL_CHANNELS_TABLE);
    }
    catch (const DbConnectionFailure& e) {
        LOG_ERROR("unable to connect to the database: " << e.what(), 0);
    }
    catch (const DbSqlQueryExecFailure& e) {
        LOG_ERROR("SQL query failure: " << e.what(), 0);
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
AccountPtr
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
        sql += " where username = \"";
        sql += userName;
        sql += "\";";
        const RecordSet& accountInfo = mDb->execSql(sql);

        // if the account is not even in the database then
        // we have no choice but to return nothing.
        if (accountInfo.isEmpty()) {
            return AccountPtr(NULL);
        }

        // create an Account instance
        // and initialize it with information about the user.
        AccountPtr account(new Account(accountInfo(0, 1),
                                       accountInfo(0, 2),
                                       accountInfo(0, 3)));

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
        sql += " where user_id = '";
        sql += accountInfo(0, 0);
        sql += "';";
        const RecordSet& charInfo = mDb->execSql(sql);

        if (!charInfo.isEmpty()) {
            Beings beings;

            LOG_INFO(userName << "'s account has " << charInfo.rows()
                << " character(s) in database.", 1);

            // As the recordset functions are set to be able to get one
            // recordset at a time, we store charInfo in a temp array of
            // strings. To avoid the problem where values of charInfo were
            // erased by the values of mapInfo.
            std::string strCharInfo[charInfo.rows()][charInfo.cols()];
            for (unsigned int i = 0; i < charInfo.rows(); ++i)
            {
                for (unsigned int j = 0; j < charInfo.cols(); ++j)
                {
                    strCharInfo[i][j] = charInfo(i,j);
                }
            }
            unsigned int charRows = charInfo.rows();

            for (unsigned int k = 0; k < charRows; ++k) {
                RawStatistics stats = {
                    toUshort(strCharInfo[k][11]), // strength
                    toUshort(strCharInfo[k][12]), // agility
                    toUshort(strCharInfo[k][13]), // vitality
                    toUshort(strCharInfo[k][14]), // intelligence
                    toUshort(strCharInfo[k][15]), // dexterity
                    toUshort(strCharInfo[k][16])  // luck
                };

                BeingPtr being(
                    new Being(strCharInfo[k][2],                     // name
                              // while the implicit type conversion from
                              // a short to an enum is invalid, the explicit
                              // type cast works :D
                              (Genders) toUshort(strCharInfo[k][3]), // gender
                              toUshort(strCharInfo[k][4]),           // hair style
                              toUshort(strCharInfo[k][5]),           // hair color
                              toUshort(strCharInfo[k][6]),           // level
                              toUint(strCharInfo[k][7]),             // money
                              stats
                ));

                unsigned int mapId = toUint(strCharInfo[k][10]);
                if ( mapId > 0 )
                {
                    being->setMapId(mapId);
                }
                else
                {
                    // Set player to default map and one of the default location
                    // Default map is to be 1, as not found return value will be 0.
                    being->setMapId((int)config.getValue("defaultMap", 1));
                }

                being->setXY(toUshort(strCharInfo[k][8]),
                             toUshort(strCharInfo[k][9]));

                mCharacters.push_back(being);
                beings.push_back(being);
            } // End of for each characters

            account->setCharacters(beings);
        } // End if there are characters.

        return account;
    }
    catch (const DbSqlQueryExecFailure& e) {
        return AccountPtr(NULL); // TODO: Throw exception here
    }
}


/**
 * Add a new account.
 */
void
DALStorage::addAccount(const AccountPtr& account)
{
    if (account.get() == 0) {
        LOG_WARN("Cannot add a NULL Account.", 0);
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
    mAccounts.insert(std::make_pair(account, ai));
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
                    mAccounts.erase(it);
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
        LOG_ERROR("SQL query failure: " << e.what(), 0);
    }
}

/**
 * Return the list of all Emails addresses.
 */
std::list<std::string>
DALStorage::getEmailList()
{
    // If not opened already
    open();

    std::list <std::string> emailList;

    try {
        std::string sql("select email from ");
        sql += ACCOUNTS_TBL_NAME;
        sql += ";";
        const dal::RecordSet& accountInfo = mDb->execSql(sql);

        // if the account is not even in the database then
        // we have no choice but to return nothing.
        if (accountInfo.isEmpty()) {
            return emailList;
        }
        for (unsigned int i = 0; i < accountInfo.rows(); i++)
        {
            // We add all these addresses to the list
            emailList.push_front(accountInfo(i, 0));
        }
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what(), 0);
    }

    return emailList;
}

/**
 * Return the number of same Emails in account's table.
 */

unsigned int
DALStorage::getSameEmailNumber(const std::string &email)
{
    // If not opened already
    open();

    try {
        std::string sql("select count(email) from ");
        sql += ACCOUNTS_TBL_NAME;
        sql += " where upper(email) = upper(\"" + email + "\");";

        const dal::RecordSet& accountInfo = mDb->execSql(sql);

        // If the account is empty then we have no choice but to return false.
        if (accountInfo.isEmpty()) {
            return 0;
        }

        std::stringstream ssStream(accountInfo(0,0));
        unsigned int iReturn = 0;
        ssStream >> iReturn;
        return iReturn;
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what(), 0);
    }

    return 0;
}

/**
 * Tells if the character's name already exists
 * @return true if character's name exists.
 */
bool
DALStorage::doesCharacterNameExists(const std::string& name)
{
    // If not opened already
    open();

    try {
            std::string sql("select count(name) from ");
            sql += CHARACTERS_TBL_NAME;
            sql += " where name = \"";
            sql += name;
            sql += "\";";
            const dal::RecordSet& accountInfo = mDb->execSql(sql);

            // if the account is empty then
            // we have no choice but to return false.
            if (accountInfo.isEmpty()) {
                return false;
            }

            std::stringstream ssStream(accountInfo(0,0));
            int iReturn = -1;
            ssStream >> iReturn;
            if ( iReturn > 0 )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what(), 0);
        }

    return false;
}

/**
 * Tells the map name from the map id
 */
const std::string
DALStorage::getMapNameFromId(const unsigned int mapId)
{
    // If not opened already
    open();

    try {
        std::stringstream sql;
        sql << "select map from ";
        sql << MAPS_TBL_NAME;
        sql << " where id = ";
        sql << mapId;
        sql << ";";

        const dal::RecordSet& mapInfo = mDb->execSql(sql.str());

        // If the map return is empty then we have no choice but to return None.
        if (mapInfo.isEmpty()) {
            return "None";
        }

        std::string strMap(mapInfo(0,0));
        return strMap;
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what(), 0);
    }

    return "None";
}

std::map<short, ChatChannel>
DALStorage::getChannelList()
{
    // If not opened already
    open();

    // specialize the string_to functor to convert
    // a string to a short.
    string_to<short> toShort;

    // The formatted datas
    std::map<short, ChatChannel> channels;

    try {
        std::stringstream sql;
        sql << "select id, name, announcement, password from ";
        sql << CHANNELS_TBL_NAME;
        sql << ";";

        const dal::RecordSet& channelInfo = mDb->execSql(sql.str());

        // If the map return is empty then we have no choice but to return false.
        if (channelInfo.isEmpty()) {
            return channels;
        }

        for ( unsigned int i = 0; i < channelInfo.rows(); ++i)
        {
            channels.insert(std::make_pair(toShort(channelInfo(i,0)),
                            ChatChannel(channelInfo(i,1),
                                        channelInfo(i,2),
                                        channelInfo(i,3))));

            LOG_DEBUG("Channel (" << channelInfo(i,0) << ") loaded: " << channelInfo(i,1)
                    << ": " << channelInfo(i,2), 5);
        }

        return channels;
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what(), 0);
    }

    return channels;
}

void
DALStorage::updateChannels(std::map<short, ChatChannel>& channelList)
{
#if defined (SQLITE_SUPPORT)
    // Reopen the db in this thread for sqlite, to avoid
    // Library Call out of sequence problem due to thread safe.
    close();
#endif
    open();

    try {
        // Empties the table
        std::stringstream sql;
        sql << "delete from "
            << CHANNELS_TBL_NAME
            << ";";

        mDb->execSql(sql.str());

        for (std::map<short, ChatChannel>::iterator i = channelList.begin();
                i != channelList.end();)
        {
            // insert registered channel if id < MAX_PUBLIC_CHANNELS_RANGE;
            if ( i->first < (signed)MAX_PUBLIC_CHANNELS_RANGE )
            {
                if (i->second.getName() != "")
                {
                    sql.str("");
                    sql << "insert into "
                        << CHANNELS_TBL_NAME
                        << " (id, name, announcement, password)"
                        << " values ("
                        << i->first << ", \""
                        << i->second.getName() << "\", \""
                        << i->second.getAnnouncement() << "\", \""
                        << i->second.getPassword() << "\");";

                        LOG_DEBUG("Channel (" << i->first << ") saved: " << i->second.getName()
                            << ": " << i->second.getAnnouncement(), 5);
                }

                mDb->execSql(sql.str());
            }
            ++i;
        }

    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what(), 0);
    }
}

/**
 * Save changes to the database permanently.
 */
void
DALStorage::flush(void)
{
    Accounts::iterator it = mAccounts.begin();
    Accounts::iterator it_end = mAccounts.end();
    for (; it != it_end; ) {
        switch ((it->second).status) {
            case AS_NEW_ACCOUNT:
                _addAccount(it->first);
                ++it;
                break;

            case AS_ACC_TO_UPDATE:
                _updAccount(it->first);
                ++it;
                break;

            case AS_ACC_TO_DELETE:
                _delAccount(it->first);
                mAccounts.erase(it++);
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
        std::string alreadyExists("table ");
        alreadyExists += tblName;
        alreadyExists += " already exists";
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
DALStorage::_addAccount(const AccountPtr& account)
{
    if (account.get() == 0) {
        return;
    }

    // assume that account is an element of mAccounts as this method is
    // private and only called by flush().

    using namespace dal;

    // TODO: we should start a transaction here so that in case of problem
    // the lost of data would be minimized.

    // insert the account.
    std::ostringstream sql1;
    sql1 << "insert into " << ACCOUNTS_TBL_NAME
         << " (username, password, email, level, banned)"
         << " values (\""
         << account->getName() << "\", \""
         << account->getPassword() << "\", \""
         << account->getEmail() << "\", "
         << account->getLevel() << ", 0);";
    mDb->execSql(sql1.str());

    // get the account id.
    std::ostringstream sql2;
    sql2 << "select id from " << ACCOUNTS_TBL_NAME
         << " where username = \"" << account->getName() << "\";";
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
    Beings& characters = account->getCharacters();

    Beings::const_iterator it = characters.begin();
    Beings::const_iterator it_end = characters.end();
    for (; it != it_end; ++it) {
        RawStatistics& stats = (*it)->getRawStatistics();
        std::ostringstream sql3;
        sql3 << "insert into " << CHARACTERS_TBL_NAME
             << " (name, gender, hair_style, hair_color, level, money, x, y, "
             << "map_id, str, agi, vit, int, dex, luck)"
             << " values ("
             << (account_it->second).id << ", \""
             << (*it)->getName() << "\", "
             << (*it)->getGender() << ", "
             << (int)(*it)->getHairStyle() << ", "
             << (int)(*it)->getHairColor() << ", "
             << (*it)->getLevel() << ", "
             << (*it)->getMoney() << ", "
             << (*it)->getX() << ", "
             << (*it)->getY() << ", "
             << (int)(*it)->getMapId() << ", "
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
DALStorage::_updAccount(const AccountPtr& account)
{
    if (account.get() == 0) {
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
        return; // Should we throw an exception here instead? No, because this can happen
                // without any bad consequences as long as we return -- Bertram.
    }

    // update the account.
    std::ostringstream sql1;
    sql1 << "update " << ACCOUNTS_TBL_NAME
         << " set username = \"" << account->getName() << "\", "
         << "password = \"" << account->getPassword() << "\", "
         << "email = \"" << account->getEmail() << "\", "
         << "level = '" << account->getLevel() << "' "
         << "where id = '" << (account_it->second).id << "';";
    mDb->execSql(sql1.str());

    // get the list of characters that belong to this account.
    Beings& characters = account->getCharacters();

    // insert or update the characters.
    Beings::const_iterator it = characters.begin();
    Beings::const_iterator it_end = characters.end();
    using namespace dal;

    for (; it != it_end; ++it) {
        // check if the character already exists in the database
        // (reminder: the character names are unique in the database).
        std::ostringstream sql2;
        sql2 << "select id from " << CHARACTERS_TBL_NAME
             << " where name = \"" << (*it)->getName() << "\";";
        const RecordSet& charInfo = mDb->execSql(sql2.str());

        RawStatistics& stats = (*it)->getRawStatistics();

        std::ostringstream sql3;
        if (charInfo.rows() == 0) {
            sql3 << "insert into " << CHARACTERS_TBL_NAME
                 << " ("
#ifdef SQLITE_SUPPORT
                 << "user_id, "
#endif
                 << "name, gender, hair_style, hair_color, level, money, x, y, map_id, str, agi, vit, int, dex, luck)"
                 << " values ("
#ifdef SQLITE_SUPPORT
                 << (account_it->second).id << ", \""
#else
                 << "\""
#endif
                 << (*it)->getName() << "\", "
                 << (*it)->getGender() << ", "
                 << (*it)->getHairStyle() << ", "
                 << (*it)->getHairColor() << ", "
                 << (*it)->getLevel() << ", "
                 << (*it)->getMoney() << ", "
                 << (*it)->getX() << ", "
                 << (*it)->getY() << ", "
                 << (*it)->getMapId() << ", "
                 << stats.strength << ", "
                 << stats.agility << ", "
                 << stats.vitality << ", "
                 << stats.intelligence << ", "
                 << stats.dexterity << ", "
                 << stats.luck << ");";
        }
        else {
            sql3 << "update " << CHARACTERS_TBL_NAME
                << " set name = \"" << (*it)->getName() << "\", "
                << " gender = " << (*it)->getGender() << ", "
                << " hair_style = " << (*it)->getHairStyle() << ", "
                << " hair_color = " << (*it)->getHairColor() << ", "
                << " level = " << (*it)->getLevel() << ", "
                << " money = " << (*it)->getMoney() << ", "
                << " x = " << (*it)->getX() << ", "
                << " y = " << (*it)->getY() << ", "
                << " map_id = " << (*it)->getMapId() << ", "
                << " str = " << stats.strength << ", "
                << " agi = " << stats.agility << ", "
                << " vit = " << stats.vitality << ", "
#if defined(MYSQL_SUPPORT) || defined(POSTGRESQL_SUPPORT)
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

    // Existing characters in memory have been inserted or updated in database.
    // Now, let's remove those who are no more in memory from database.

    // specialize the string_to functor to convert
    // a string to an unsigned int.
    string_to<unsigned short> toUint;

    std::ostringstream sql4;
    sql4 << "select name, id from " << CHARACTERS_TBL_NAME
         << " where user_id = '" << (account_it->second).id << "';";
    const RecordSet& charInMemInfo = mDb->execSql(sql4.str());

    // We compare chars from memory and those existing in db,
    // And delete those not in mem but existing in db.
    bool charFound;
    for ( unsigned int i = 0; i < charInMemInfo.rows(); ++i) // in database
    {
        charFound = false;
        it = characters.begin();
        for (; it != it_end; ++it) // In memory
        {
            if ( charInMemInfo(i, 0) == (*it)->getName() )
            {
                charFound = true;
                break;
            }
        }
        if ( !charFound )
        {
            // The char is db but not in memory,
            // It will be removed from database.
            // We store the id of the char to delete
            // Because as deleted, the RecordSet is also emptied
            // That creates an error.
            unsigned int charId = toUint(charInMemInfo(i, 1));

                // delete the inventory.
                std::ostringstream sql5;
                sql5 << "delete from ";
                sql5 << INVENTORIES_TBL_NAME;
                sql5 << " where owner_id = '";
                sql5 << charId;
                sql5 << "';";
                mDb->execSql(sql5.str());

                // now delete the character.
                std::ostringstream sql6;
                sql6 << "delete from ";
                sql6 << CHARACTERS_TBL_NAME;
                sql6 << " where id = '";
                sql6 << charId;
                sql6 << "';";
                mDb->execSql(sql6.str());
        }
    }
}


/**
 * Delete an account and its associated data from the database.
 */
void
DALStorage::_delAccount(const AccountPtr& account)
{
    if (account.get() != 0) {
        _delAccount(account->getName());
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
    sql += " where username = \"";
    sql += userName;
    sql += "\";";
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
    // db.set-transaction-type of this-db to db.manual-commit, for instance
    // Agreed, but will sqlite support this ?

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
