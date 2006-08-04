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

#include <cassert>

#include "dalstorage.h"

#include "configuration.h"
#include "dalstoragesql.h"

#include "dal/dalexcept.h"
#include "dal/dataproviderfactory.h"

#include "utils/cipher.h"
#include "utils/functors.h"
#include "utils/logger.h"

/**
 * Functor used to search an Account by name in Accounts.
 */
class account_by_name
{
    public:
        account_by_name(const std::string& name)
            : mName(name)
        {}

        bool operator()(std::pair<unsigned, AccountPtr> const &elem) const
        { return elem.second->getName() == mName; }

    private:
        std::string mName; /**< the name to look for */
};


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
    Accounts::iterator it_end = mAccounts.end(),
        it = std::find_if(mAccounts.begin(), it_end, account_by_name(userName));

    if (it != it_end)
        return it->second;

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

        // specialize the string_to functor to convert
        // a string to an unsigned int.
        string_to<unsigned short> toUint;
        unsigned id = toUint(accountInfo(0, 0));

        // create an Account instance
        // and initialize it with information about the user.
        AccountPtr account(new Account(accountInfo(0, 1),
                                       accountInfo(0, 2),
                                       accountInfo(0, 3), id));

        // specialize the string_to functor to convert
        // a string to an unsigned short.
        string_to<unsigned short> toUshort;

        mAccounts.insert(std::make_pair(id, account));

        // load the characters associated with the account.
        sql = "select * from ";
        sql += CHARACTERS_TBL_NAME;
        sql += " where user_id = '";
        sql += accountInfo(0, 0);
        sql += "';";
        const RecordSet& charInfo = mDb->execSql(sql);

        if (!charInfo.isEmpty()) {
            Players players;

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
                PlayerPtr player(new Player(strCharInfo[k][2], toUint(strCharInfo[k][0])));
                player->setGender((Genders)toUshort(strCharInfo[k][3]));
                player->setHairStyle(toUshort(strCharInfo[k][4]));
                player->setHairColor(toUshort(strCharInfo[k][5]));
                player->setLevel(toUshort(strCharInfo[k][6]));
                player->setMoney(toUint(strCharInfo[k][7]));
                player->setXY(toUshort(strCharInfo[k][8]),
                              toUshort(strCharInfo[k][9]));
                for (int i = 0; i < NB_RSTAT; ++i)
                    player->setRawStat(i, toUshort(strCharInfo[k][11 + i]));

                unsigned int mapId = toUint(strCharInfo[k][10]);
                if ( mapId > 0 )
                {
                    player->setMapId(mapId);
                }
                else
                {
                    // Set player to default map and one of the default location
                    // Default map is to be 1, as not found return value will be 0.
                    player->setMapId((int)config.getValue("defaultMap", 1));
                }

                mCharacters.push_back(player);
                players.push_back(player);
            } // End of for each characters

            account->setCharacters(players);
        } // End if there are characters.

        return account;
    }
    catch (const DbSqlQueryExecFailure& e) {
        return AccountPtr(NULL); // TODO: Throw exception here
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
 * Tells if the email address already exists
 * @return true if the email address exists.
 */
bool DALStorage::doesEmailAddressExist(std::string const &email)
{
    // If not opened already
    open();

    try {
        std::ostringstream sql;
        sql << "select count(email) from " << ACCOUNTS_TBL_NAME
            << " where upper(email) = upper(\"" << email << "\");";
        dal::RecordSet const &accountInfo = mDb->execSql(sql.str());

        std::istringstream ssStream(accountInfo(0, 0));
        unsigned int iReturn = 1;
        ssStream >> iReturn;
        return iReturn != 0;
    } catch (std::exception const &e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what(), 0);
    }

    return true;
}

/**
 * Tells if the character's name already exists
 * @return true if character's name exists.
 */
bool DALStorage::doesCharacterNameExist(const std::string& name)
{
    // If not opened already
    open();

    try {
        std::ostringstream sql;
        sql << "select count(name) from " << CHARACTERS_TBL_NAME
            << " where name = \"" << name << "\";";
        dal::RecordSet const &accountInfo = mDb->execSql(sql.str());

        std::istringstream ssStream(accountInfo(0, 0));
        int iReturn = 1;
        ssStream >> iReturn;
        return iReturn != 0;
    } catch (std::exception const &e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what(), 0);
    }

    return true;
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
void DALStorage::addAccount(AccountPtr const &account)
{
    assert(account->getCharacters().size() == 0);

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
    unsigned id = toUint(accountInfo(0, 0));
    account->setID(id);
    mAccounts.insert(std::make_pair(id, account));
}

/**
 * Update all the accounts from the database.
 */
void DALStorage::flushAll()
{
    for (Accounts::iterator i = mAccounts.begin(),
         i_end = mAccounts.end(); i != i_end; ++i)
        flush(i->second);
}

/**
 * Update an account from the database.
 */
void DALStorage::flush(AccountPtr const &account)
{
    assert(account->getID() >= 0);

    using namespace dal;

    // TODO: we should start a transaction here so that in case of problem
    // the loss of data would be minimized.

    // update the account.
    std::ostringstream sql1;
    sql1 << "update " << ACCOUNTS_TBL_NAME
         << " set username = \"" << account->getName() << "\", "
         << "password = \"" << account->getPassword() << "\", "
         << "email = \"" << account->getEmail() << "\", "
         << "level = '" << account->getLevel() << "' "
         << "where id = '" << account->getID() << "';";
    mDb->execSql(sql1.str());

    // get the list of characters that belong to this account.
    Players &characters = account->getCharacters();

    // insert or update the characters.
    for (Players::const_iterator it = characters.begin(),
         it_end = characters.end(); it != it_end; ++it) {

        std::ostringstream sql3;
        if ((*it)->getID() < 0) {
            // insert the character
            sql3 << "insert into " << CHARACTERS_TBL_NAME
                 << " (user_id, name, gender, hair_style, hair_color, level, money,"
                    " x, y, map_id, str, agi, vit, int, dex, luck) values ("
                 << account->getID() << ", \""
                 << (*it)->getName() << "\", "
                 << (*it)->getGender() << ", "
                 << (int)(*it)->getHairStyle() << ", "
                 << (int)(*it)->getHairColor() << ", "
                 << (int)(*it)->getLevel() << ", "
                 << (*it)->getMoney() << ", "
                 << (*it)->getX() << ", "
                 << (*it)->getY() << ", "
                 << (*it)->getMapId() << ", "
                 << (*it)->getRawStat(STAT_STR) << ", "
                 << (*it)->getRawStat(STAT_AGI) << ", "
                 << (*it)->getRawStat(STAT_VIT) << ", "
                 << (*it)->getRawStat(STAT_INT) << ", "
                 << (*it)->getRawStat(STAT_DEX) << ", "
                 << (*it)->getRawStat(STAT_LUK) << ");";

            // get the character id
            std::ostringstream sql2;
            sql2 << "select id from " << CHARACTERS_TBL_NAME
                 << " where name = \"" << (*it)->getName() << "\";";
            RecordSet const &charInfo = mDb->execSql(sql2.str());
            string_to<unsigned int> toUint;
            (*it)->setID(toUint(charInfo(0, 0)));
        } else {
            sql3 << "update " << CHARACTERS_TBL_NAME
                << " set name = \"" << (*it)->getName() << "\", "
                << " gender = " << (*it)->getGender() << ", "
                << " hair_style = " << (int)(*it)->getHairStyle() << ", "
                << " hair_color = " << (int)(*it)->getHairColor() << ", "
                << " level = " << (int)(*it)->getLevel() << ", "
                << " money = " << (*it)->getMoney() << ", "
                << " x = " << (*it)->getX() << ", "
                << " y = " << (*it)->getY() << ", "
                << " map_id = " << (*it)->getMapId() << ", "
                << " str = " << (*it)->getRawStat(STAT_STR) << ", "
                << " agi = " << (*it)->getRawStat(STAT_AGI) << ", "
                << " vit = " << (*it)->getRawStat(STAT_VIT) << ", "
#if defined(MYSQL_SUPPORT) || defined(POSTGRESQL_SUPPORT)
                << " `int` = " << (*it)->getRawStat(STAT_INT) << ", "
#else
                << " int = " << (*it)->getRawStat(STAT_INT) << ", "
#endif
                << " dex = " << (*it)->getRawStat(STAT_DEX) << ", "
                << " luck = " << (*it)->getRawStat(STAT_LUK)
                << " where id = " << (*it)->getID() << ";";
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
         << " where user_id = '" << account->getID() << "';";
    const RecordSet& charInMemInfo = mDb->execSql(sql4.str());

    // We compare chars from memory and those existing in db,
    // And delete those not in mem but existing in db.
    bool charFound;
    for ( unsigned int i = 0; i < charInMemInfo.rows(); ++i) // in database
    {
        charFound = false;
        for (Players::const_iterator it = characters.begin(),
             it_end = characters.end(); it != it_end; ++it) // In memory
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
void DALStorage::delAccount(AccountPtr const &account)
{
    using namespace dal;

    account->setCharacters(Players());
    flush(account);
    mAccounts.erase(account->getID());

    // delete the account.
    std::ostringstream sql;
    sql << "delete from " << ACCOUNTS_TBL_NAME
        << " where id = '" << account->getID() << "';";
    mDb->execSql(sql.str());
}

/**
 * Unload an account from memory.
 */
void DALStorage::unloadAccount(AccountPtr const &account)
{
    flush(account);
    mAccounts.erase(account->getID());
}
