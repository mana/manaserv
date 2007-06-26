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

#include "account-server/dalstorage.hpp"

#include <cassert>

#include "configuration.h"
#include "point.h"
#include "account-server/characterdata.hpp"
#include "account-server/guild.hpp"
#include "account-server/guildmanager.hpp"
#include "account-server/dalstoragesql.hpp"
#include "dal/dalexcept.h"
#include "dal/dataproviderfactory.h"
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
 * Functor used to search a character by ID in Characters.
 */
class character_by_id
{
    public:
        character_by_id(int id)
            : mID(id)
        {}

        bool operator()(CharacterPtr const &elem) const
        { return elem->getDatabaseID() == mID; }

    private:
        int mID; /**< the ID to look for */
};

/**
* Functor used to search a character by name in Characters.
 */
class character_by_name
{
public:
    character_by_name(const std::string &name)
    : mName(name)
    {}
    
    bool operator()(CharacterPtr const &elem) const
    { return elem->getName() == mName; }
    
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
            LOG_INFO("Using " << dbFile << " as Database Name.");
            dbFileShown = true;
        }
#elif defined (SQLITE_SUPPORT)
        // create the database file name.
        dbFile += ".db";
        mDb->connect(dbFile, "", "");
        if (!dbFileShown)
        {
            LOG_INFO("SQLite uses ./" << dbFile << " as DB.");
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

        // FIXME: The tables should be checked/created at startup in order to
        // avoid a DbSqlQueryExecFailure assert on sqlite while registering.
        // Also, this would initialize connection to the database earlier in
        // memory.

        createTable(ACCOUNTS_TBL_NAME, SQL_ACCOUNTS_TABLE);
        createTable(CHARACTERS_TBL_NAME, SQL_CHARACTERS_TABLE);
        createTable(ITEMS_TBL_NAME, SQL_ITEMS_TABLE);
        createTable(WORLD_ITEMS_TBL_NAME, SQL_WORLD_ITEMS_TABLE);
        createTable(INVENTORIES_TBL_NAME, SQL_INVENTORIES_TABLE);
        createTable(CHANNELS_TBL_NAME, SQL_CHANNELS_TABLE);
        createTable(GUILDS_TBL_NAME, SQL_GUILDS_TABLE);
        createTable(GUILD_MEMBERS_TBL_NAME, SQL_GUILD_MEMBERS_TABLE);
    }
    catch (const DbConnectionFailure& e) {
        LOG_ERROR("(DALStorage::open #1) Unable to connect to the database: "
                     << e.what());
    }
    catch (const DbSqlQueryExecFailure& e) {
        LOG_ERROR("(DALStorage::open #2) SQL query failure: " << e.what());
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
        std::ostringstream sql;
        sql << "select * from " << ACCOUNTS_TBL_NAME << " where username = \""
            << userName << "\";";
        const RecordSet& accountInfo = mDb->execSql(sql.str());

        // if the account is not even in the database then
        // we have no choice but to return nothing.
        if (accountInfo.isEmpty()) {
            return AccountPtr(NULL);
        }

        // specialize the string_to functor to convert
        // a string to an unsigned int.
        string_to< unsigned > toUint;
        unsigned id = toUint(accountInfo(0, 0));

        // create an Account instance
        // and initialize it with information about the user.
        AccountPtr account(new Account(accountInfo(0, 1),
                                       accountInfo(0, 2),
                                       accountInfo(0, 3), id));

        mAccounts.insert(std::make_pair(id, account));

        // load the characters associated with the account.
        sql.str(std::string());
        sql << "select id from " << CHARACTERS_TBL_NAME << " where user_id = '"
            << accountInfo(0, 0) << "';";
        RecordSet const &charInfo = mDb->execSql(sql.str());

        if (!charInfo.isEmpty())
        {
            int size = charInfo.rows();
            Characters characters;

            LOG_DEBUG(userName << "'s account has " << size
                      << " character(s) in database.");

            // Two steps: it seems like multiple requests cannot be alive at the same time.
            std::vector< unsigned > characterIDs;
            for (int k = 0; k < size; ++k)
            {
                characterIDs.push_back(toUint(charInfo(k, 0)));
            }

            for (int k = 0; k < size; ++k)
            {
                characters.push_back(getCharacter(characterIDs[k]));
            }

            account->setCharacters(characters);
        }

        return account;
    }
    catch (const DbSqlQueryExecFailure& e)
    {
        return AccountPtr(NULL); // TODO: Throw exception here
    }
}

/**
 * Get an account by ID.
 */
AccountPtr
DALStorage::getAccountByID(int accountID)
{
    // connect to the database (if not connected yet).
    open();

    // look for the account in the list first.
    Accounts::iterator it = mAccounts.find(accountID);

    if (it != mAccounts.end())
        return it->second;

    using namespace dal;

    // the account was not in the list, look for it in the database.
    try {
        std::ostringstream sql;
        sql << "select * from " << ACCOUNTS_TBL_NAME << " where id = '"
            << accountID << "';";
        const RecordSet& accountInfo = mDb->execSql(sql.str());

        // if the account is not even in the database then
        // we have no choice but to return nothing.
        if (accountInfo.isEmpty()) {
            return AccountPtr(NULL);
        }

        // specialize the string_to functor to convert
        // a string to an unsigned int.
        string_to< unsigned > toUint;
        unsigned id = toUint(accountInfo(0, 0));

        // create an Account instance
        // and initialize it with information about the user.
        AccountPtr account(new Account(accountInfo(0, 1),
                                       accountInfo(0, 2),
                                       accountInfo(0, 3), id));

        mAccounts.insert(std::make_pair(id, account));

        // load the characters associated with the account.
        sql.str(std::string());
        sql << "select id from " << CHARACTERS_TBL_NAME << " where user_id = '"
            << accountInfo(0, 0) << "';";
        RecordSet const &charInfo = mDb->execSql(sql.str());

        if (!charInfo.isEmpty())
        {
            int size = charInfo.rows();
            Characters characters;

            LOG_DEBUG("AccountID: "<< accountID << "; has " << size
                      << " character(s) in database.");

            // Two steps: it seems like multiple requests cannot be alive at the same time.
            std::vector< unsigned > characterIDs;
            for (int k = 0; k < size; ++k)
            {
                characterIDs.push_back(toUint(charInfo(k, 0)));
            }

            for (int k = 0; k < size; ++k)
            {
                characters.push_back(getCharacter(characterIDs[k]));
            }

            account->setCharacters(characters);
        }

        return account;
    }
    catch (const DbSqlQueryExecFailure& e)
    {
        return AccountPtr(NULL); // TODO: Throw exception here
    }
}

/**
 * Gets a character by database ID.
 */
CharacterPtr DALStorage::getCharacter(int id)
{
    // connect to the database (if not connected yet).
    open();

    // look for the character in the list first.
    Characters::iterator it_end = mCharacters.end(),
        it = std::find_if(mCharacters.begin(), it_end, character_by_id(id));

    if (it != it_end)
        return *it;

    using namespace dal;

    // the account was not in the list, look for it in the database.
    try {
        std::ostringstream sql;
        sql << "select * from " << CHARACTERS_TBL_NAME << " where id = '"
            << id << "';";
        RecordSet const &charInfo = mDb->execSql(sql.str());

        // if the character is not even in the database then
        // we have no choice but to return nothing.
        if (charInfo.isEmpty())
        {
            return CharacterPtr(NULL);
        }

        // specialize the string_to functor to convert
        // a string to an unsigned int.
        string_to< unsigned > toUint;

        // specialize the string_to functor to convert
        // a string to an unsigned short.
        string_to< unsigned short > toUshort;

        CharacterData *character = new CharacterData(charInfo(0, 2),
                                                      toUint(charInfo(0, 0)));
        character->setAccountID(toUint(charInfo(0, 1)));
        character->setGender(toUshort(charInfo(0, 3)));
        character->setHairStyle(toUshort(charInfo(0, 4)));
        character->setHairColor(toUshort(charInfo(0, 5)));
        character->setLevel(toUshort(charInfo(0, 6)));
        character->setMoney(toUint(charInfo(0, 7)));
        Point pos(toUshort(charInfo(0, 8)), toUshort(charInfo(0, 9)));
        character->setPosition(pos);
        for (int i = 0; i < NB_BASE_ATTRIBUTES; ++i)
        {
            character->setBaseAttribute(i, toUshort(charInfo(0, 11 + i)));
        }

        int mapId = toUint(charInfo(0, 10));
        if (mapId > 0)
        {
            character->setMapId(mapId);
        }
        else
        {
            // Set character to default map and one of the default location
            // Default map is to be 1, as not found return value will be 0.
            character->setMapId((int)config.getValue("defaultMap", 1));
        }

        CharacterPtr ptr(character);
        mCharacters.push_back(ptr);
        return ptr;
    }
    catch (const DbSqlQueryExecFailure& e)
    {
        return CharacterPtr(NULL); // TODO: Throw exception here
    }
}

/**
* Gets a character by character name.
 */
CharacterPtr DALStorage::getCharacter(const std::string &name)
{
    // connect to the database (if not connected yet).
    open();
    
    // look for the character in the list first.
    Characters::iterator it_end = mCharacters.end(),
    it = std::find_if(mCharacters.begin(), it_end, character_by_name(name));
    
    if (it != it_end)
        return *it;
    
    using namespace dal;
    
    // the account was not in the list, look for it in the database.
    try {
        std::ostringstream sql;
        sql << "select * from " << CHARACTERS_TBL_NAME << " where name = '"
            << name << "';";
        RecordSet const &charInfo = mDb->execSql(sql.str());
        
        // if the character is not even in the database then
        // we have no choice but to return nothing.
        if (charInfo.isEmpty())
        {
            return CharacterPtr(NULL);
        }
        
        // specialize the string_to functor to convert
        // a string to an unsigned int.
        string_to< unsigned > toUint;
        
        // specialize the string_to functor to convert
        // a string to an unsigned short.
        string_to< unsigned short > toUshort;
        
        CharacterData *character = new CharacterData(charInfo(0, 2),
                                                     toUint(charInfo(0, 0)));
        character->setAccountID(toUint(charInfo(0, 1)));
        character->setGender(toUshort(charInfo(0, 3)));
        character->setHairStyle(toUshort(charInfo(0, 4)));
        character->setHairColor(toUshort(charInfo(0, 5)));
        character->setLevel(toUshort(charInfo(0, 6)));
        character->setMoney(toUint(charInfo(0, 7)));
        Point pos(toUshort(charInfo(0, 8)), toUshort(charInfo(0, 9)));
        character->setPosition(pos);
        for (int i = 0; i < NB_BASE_ATTRIBUTES; ++i)
        {
            character->setBaseAttribute(i, toUshort(charInfo(0, 11 + i)));
        }
        
        int mapId = toUint(charInfo(0, 10));
        if (mapId > 0)
        {
            character->setMapId(mapId);
        }
        else
        {
            // Set character to default map and one of the default location
            // Default map is to be 1, as not found return value will be 0.
            character->setMapId((int)config.getValue("defaultMap", 1));
        }
        
        CharacterPtr ptr(character);
        mCharacters.push_back(ptr);
        return ptr;
    }
    catch (const DbSqlQueryExecFailure& e)
    {
        return CharacterPtr(NULL); // TODO: Throw exception here
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
        LOG_ERROR("(DALStorage::getEmailList) SQL query failure: " << e.what());
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
        LOG_ERROR("(DALStorage::doesEmailAddressExist) SQL query failure: " << e.what());
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
        LOG_ERROR("(DALStorage::doesCharacterNameExist) SQL query failure: "
                << e.what());
    }

    return true;
}

bool
DALStorage::updateCharacter(CharacterPtr character)
{
    // If not opened already
    open();

    // Update the database Character data (see CharacterData for details)
    try
    {
        std::ostringstream sqlUpdateCharacterInfo;
        sqlUpdateCharacterInfo
            << "update "        << CHARACTERS_TBL_NAME << " "
            << "set "
            << "gender = '"     << character->getGender()
                               << "', "
            << "hair_style = '" << (int)character->getHairStyle()
                               << "', "
            << "hair_color = '" << (int)character->getHairColor()
                               << "', "
            << "level = '"      << (int)character->getLevel()
                               << "', "
            << "money = '"      << character->getMoney()
                               << "', "
            << "x = '"          << character->getPosition().x
                               << "', "
            << "y = '"          << character->getPosition().y
                               << "', "
            << "map_id = '"     << character->getMapId()
                               << "', "
            << "str = '"        << character->getBaseAttribute(BASE_ATTR_STRENGTH)
                               << "', "
            << "agi = '"        << character->getBaseAttribute(BASE_ATTR_AGILITY)
                               << "', "
            << "dex = '"        << character->getBaseAttribute(BASE_ATTR_DEXTERITY)
                               << "', "
            << "vit = '"        << character->getBaseAttribute(BASE_ATTR_VITALITY)
                               << "', "
#if defined(MYSQL_SUPPORT) || defined(POSTGRESQL_SUPPORT)
            << "`int` = '"
#else
            << "int = '"
#endif
                               << character->getBaseAttribute(BASE_ATTR_INTELLIGENCE)
                               << "', "

            << "will = '"       << character->getBaseAttribute(BASE_ATTR_WILLPOWER)
                                << "', "
            << "charisma = '"   << character->getBaseAttribute(BASE_ATTR_CHARISMA)
                                << "' "
            << "where id = '"   << character->getDatabaseID()
                               << "';";

        mDb->execSql(sqlUpdateCharacterInfo.str());
    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception.
        LOG_ERROR("(DALStorage::updateCharacter #1) SQL query failure: " << e.what());
        return false;
    }

    /**
     *  Character's inventory
     */

    // Delete the old inventory first
    try
    {
        std::ostringstream sqlDeleteCharacterInventory;
        sqlDeleteCharacterInventory
            << "delete from " << INVENTORIES_TBL_NAME
            << " where owner_id = '" << character->getDatabaseID() << "';";
        mDb->execSql(sqlDeleteCharacterInventory.str());
    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception.
        LOG_ERROR("(DALStorage::updateCharacter #2) SQL query failure: " << e.what());
        return false;
    }

    // Insert the new inventory data
    if (character->getNumberOfInventoryItems())
    {
        try
        {
            std::ostringstream sqlInsertCharacterInventory;

            sqlInsertCharacterInventory
                << "insert into " << INVENTORIES_TBL_NAME
                << " (owner_id, class_id, amount, equipped) "
                << "values ";

            for (int j = 0; j < character->getNumberOfInventoryItems(); j++)
            {
                sqlInsertCharacterInventory
                    << "(" << character->getDatabaseID() << ", "
                    << character->getInventoryItem(j).itemClassId << ", "
                    << character->getInventoryItem(j).numberOfItemsInSlot
                    << ", "
                    << (unsigned short)
                       character->getInventoryItem(j).isEquiped
                    << ")";

                // Adding the comma only if it's needed
                if (j < (character->getNumberOfInventoryItems() - 1))
                        sqlInsertCharacterInventory << ", ";
            }
            sqlInsertCharacterInventory << ";";

            mDb->execSql(sqlInsertCharacterInventory.str());
        }
        catch (const dal::DbSqlQueryExecFailure& e)
        {
            // TODO: throw an exception.
            LOG_ERROR("(DALStorage::updateCharacter #3) SQL query failure: " << e.what());
            return false;
        }
    }
    
    return true;
}

std::map<short, ChatChannel>
DALStorage::getChannelList()
{
    // If not opened already
    open();

    // specialize the string_to functor to convert
    // a string to a short.
    string_to<short> toShort;
    string_to<bool> toBool;

    // The formatted datas
    std::map<short, ChatChannel> channels;

    try {
        std::stringstream sql;
        sql << "select id, name, announcement, password, privacy from ";
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
                                        channelInfo(i,3),
                                        toBool(channelInfo(i,4)))));

            LOG_DEBUG("Channel (" << channelInfo(i,0) << ") loaded: " << channelInfo(i,1)
                      << ": " << channelInfo(i,2));
        }

        return channels;
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("(DALStorage::getChannelList) SQL query failure: " << e.what());
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
                        << " (id, name, announcement, password, privacy)"
                        << " values ("
                        << i->first << ", \""
                        << i->second.getName() << "\", \""
                        << i->second.getAnnouncement() << "\", \""
                        << i->second.getPassword() << "\", \""
                        << i->second.getPrivacy() << "\");";

                        LOG_DEBUG("Channel (" << i->first << ") saved: "
                                  << i->second.getName()
                                  << ": " << i->second.getAnnouncement());
                }

                mDb->execSql(sql.str());
            }
            ++i;
        }

    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("(DALStorage::updateChannels) SQL query failure: " << e.what());
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
    std::ostringstream sqlUpdateAccountTable;
    sqlUpdateAccountTable << "update " << ACCOUNTS_TBL_NAME
         << " set username = \"" << account->getName() << "\", "
         << "password = \"" << account->getPassword() << "\", "
         << "email = \"" << account->getEmail() << "\", "
         << "level = '" << account->getLevel() << "' "
         << "where id = '" << account->getID() << "';";
    mDb->execSql(sqlUpdateAccountTable.str());

    // get the list of characters that belong to this account.
    Characters &characters = account->getCharacters();

    // insert or update the characters.
    for (Characters::const_iterator it = characters.begin(),
         it_end = characters.end(); it != it_end; ++it)
    {
        if ((*it)->getDatabaseID() < 0) {
            std::ostringstream sqlInsertCharactersTable;
            // insert the character
            // This assumes that the characters name has been checked for
            // uniqueness
            sqlInsertCharactersTable
                 << "insert into " << CHARACTERS_TBL_NAME
                 << " (user_id, name, gender, hair_style, hair_color, level, money,"
                 << " x, y, map_id, str, agi, dex, vit, int, will, charisma) values ("
                 << account->getID() << ", \""
                 << (*it)->getName() << "\", "
                 << (*it)->getGender() << ", "
                 << (int)(*it)->getHairStyle() << ", "
                 << (int)(*it)->getHairColor() << ", "
                 << (int)(*it)->getLevel() << ", "
                 << (*it)->getMoney() << ", "
                 << (*it)->getPosition().x << ", "
                 << (*it)->getPosition().y << ", "
                 << (*it)->getMapId() << ", "
                 << (*it)->getBaseAttribute(BASE_ATTR_STRENGTH) << ", "
                 << (*it)->getBaseAttribute(BASE_ATTR_AGILITY) << ", "
                 << (*it)->getBaseAttribute(BASE_ATTR_DEXTERITY) << ", "
                 << (*it)->getBaseAttribute(BASE_ATTR_VITALITY) << ", "
                 << (*it)->getBaseAttribute(BASE_ATTR_INTELLIGENCE) << ", "
                 << (*it)->getBaseAttribute(BASE_ATTR_WILLPOWER) << ", "
                 << (*it)->getBaseAttribute(BASE_ATTR_CHARISMA) << ");";

            mDb->execSql(sqlInsertCharactersTable.str());
        } else {
            std::ostringstream sqlUpdateCharactersTable;
            sqlUpdateCharactersTable
                << "update " << CHARACTERS_TBL_NAME
                << " set name = \"" << (*it)->getName() << "\", "
                << " gender = " << (*it)->getGender() << ", "
                << " hair_style = " << (int)(*it)->getHairStyle() << ", "
                << " hair_color = " << (int)(*it)->getHairColor() << ", "
                << " level = " << (int)(*it)->getLevel() << ", "
                << " money = " << (*it)->getMoney() << ", "
                << " x = " << (*it)->getPosition().x << ", "
                << " y = " << (*it)->getPosition().y << ", "
                << " map_id = " << (*it)->getMapId() << ", "
                << " str = " << (*it)->getBaseAttribute(BASE_ATTR_STRENGTH) << ", "
                << " agi = " << (*it)->getBaseAttribute(BASE_ATTR_AGILITY) << ", "
                << " dex = " << (*it)->getBaseAttribute(BASE_ATTR_DEXTERITY) << ", "
                << " vit = " << (*it)->getBaseAttribute(BASE_ATTR_VITALITY) << ", "
#if defined(MYSQL_SUPPORT) || defined(POSTGRESQL_SUPPORT)
                << " `int` = "
#else
                << " int = "
#endif
                               << (*it)->getBaseAttribute(BASE_ATTR_INTELLIGENCE) << ", "
                << " will = " << (*it)->getBaseAttribute(BASE_ATTR_WILLPOWER) << ", "
                << " charisma = " << (*it)->getBaseAttribute(BASE_ATTR_CHARISMA)
                << " where id = " << (*it)->getDatabaseID() << ";";

            mDb->execSql(sqlUpdateCharactersTable.str());
        }

        if ((*it)->getDatabaseID() < 0)
        {
            // get the character's id
            std::ostringstream sqlSelectIdCharactersTable;
            sqlSelectIdCharactersTable
                 << "select id from " << CHARACTERS_TBL_NAME
                 << " where name = \"" << (*it)->getName() << "\";";
            RecordSet const &charInfo =
                mDb->execSql(sqlSelectIdCharactersTable.str());

            if (!charInfo.isEmpty()) {
                string_to<unsigned int> toUint;
                (*it)->setDatabaseID(toUint(charInfo(0, 0)));
            }
            else
            {
                // TODO: The character's name is not unique, or some other
                // error has occured
            }
        }

        // TODO: inventories.
    }

    // Existing characters in memory have been inserted or updated in database.
    // Now, let's remove those who are no more in memory from database.

    // specialize the string_to functor to convert
    // a string to an unsigned int.
    string_to<unsigned short> toUint;

    std::ostringstream sqlSelectNameIdCharactersTable;
    sqlSelectNameIdCharactersTable
         << "select name, id from " << CHARACTERS_TBL_NAME
         << " where user_id = '" << account->getID() << "';";
    const RecordSet& charInMemInfo =
        mDb->execSql(sqlSelectNameIdCharactersTable.str());

    // We compare chars from memory and those existing in db,
    // And delete those not in mem but existing in db.
    bool charFound;
    for (unsigned int i = 0; i < charInMemInfo.rows(); ++i) // in database
    {
        charFound = false;
        for (Characters::const_iterator it = characters.begin(),
             it_end = characters.end(); it != it_end; ++it) // In memory
        {
            if (charInMemInfo(i, 0) == (*it)->getName())
            {
                charFound = true;
                break;
            }
        }
        if (!charFound)
        {
            // The char is db but not in memory,
            // It will be removed from database.
            // We store the id of the char to delete
            // Because as deleted, the RecordSet is also emptied
            // That creates an error.
            unsigned int charId = toUint(charInMemInfo(i, 1));

                // delete the inventory.
                std::ostringstream sqlDeleteInventoryTable;
                sqlDeleteInventoryTable
                    << "delete from "
                    << INVENTORIES_TBL_NAME
                    << " where owner_id = '"
                    << charId
                    << "';";
                mDb->execSql(sqlDeleteInventoryTable.str());

                // now delete the character.
                std::ostringstream sqlDeleteCharactersTable;
                sqlDeleteCharactersTable
                    << "delete from "
                    << CHARACTERS_TBL_NAME
                    << " where id = '"
                    << charId
                    << "';";
                mDb->execSql(sqlDeleteCharactersTable.str());
        }
    }
}


/**
 * Delete an account and its associated data from the database.
 */
void DALStorage::delAccount(AccountPtr const &account)
{
    using namespace dal;

    account->setCharacters(Characters());
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

/**
 * Add a guild
 */
void DALStorage::addGuild(Guild* guild)
{
#if defined (SQLITE_SUPPORT)
    // Reopen the db in this thread for sqlite, to avoid
    // Library Call out of sequence problem due to thread safe.
    close();
#endif
    open();
    
    std::ostringstream insertSql;
    insertSql << "insert into " << GUILDS_TBL_NAME
        << " (name) "
        << " values (\""
        << guild->getName() << "\");";
    mDb->execSql(insertSql.str());

    std::ostringstream selectSql;
    selectSql << "select id from " << GUILDS_TBL_NAME
        << " where name = \"" << guild->getName() << "\";";
    const dal::RecordSet& guildInfo = mDb->execSql(selectSql.str());
    string_to<unsigned int> toUint;
    unsigned id = toUint(guildInfo(0, 0));
    guild->setId(id);
}

/**
 * Remove guild
 */
void DALStorage::removeGuild(Guild* guild)
{
#if defined (SQLITE_SUPPORT)
    // Reopen the db in this thread for sqlite, to avoid
    // Library Call out of sequence problem due to thread safe.
    close();
#endif
    open();
    
    std::ostringstream sql;
    sql << "delete from " << GUILDS_TBL_NAME
        << " where id = '"
        << guild->getId() << "';";
    mDb->execSql(sql.str());
}

/**
 * add a member to a guild
 */
void DALStorage::addGuildMember(int guildId, const std::string &memberName)
{
#if defined (SQLITE_SUPPORT)
    // Reopen the db in this thread for sqlite, to avoid
    // Library Call out of sequence problem due to thread safe.
    close();
#endif
    open();
    
    std::ostringstream sql;
    
    try
    {
        sql << "insert into " << GUILD_MEMBERS_TBL_NAME
        << " (guild_id, member_name)"
        << " values ("
        << guildId << ", \""
        << memberName << "\");";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what());
    }    
}

/**
* remove a member from a guild
 */
void DALStorage::removeGuildMember(int guildId, const std::string &memberName)
{
#if defined (SQLITE_SUPPORT)
    // Reopen the db in this thread for sqlite, to avoid
    // Library Call out of sequence problem due to thread safe.
    close();
#endif
    open();
    
    std::ostringstream sql;
    
    try
    {
        sql << "delete from " << GUILD_MEMBERS_TBL_NAME
        << " where member_name = \""
        << memberName << "\" and guild_id = '"
        << guildId << "';";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what());
    }    
}

/**
 * get a list of guilds
 */
std::list<Guild*> DALStorage::getGuildList()
{
#if defined (SQLITE_SUPPORT)
    // Reopen the db in this thread for sqlite, to avoid
    // Library Call out of sequence problem due to thread safe.
    close();
#endif
    open();
    
    std::list<Guild*> guilds;
    std::stringstream sql;
    string_to<short> toShort;

    /**
     * Get the guilds stored in the db.
     */
    
    try
    {
        sql << "select id, name from " << GUILDS_TBL_NAME << ";";
        const dal::RecordSet& guildInfo = mDb->execSql(sql.str());
        
        // check that at least 1 guild was returned
        if(guildInfo.isEmpty())
        {
            return guilds;
        }
        
        // loop through every row in the table and assign it to a guild
        for ( unsigned int i = 0; i < guildInfo.rows(); ++i)
        {
            Guild* guild = new Guild(guildInfo(i,1));
            guild->setId(toShort(guildInfo(i,0)));
            guilds.push_back(guild);
        }
        
        /**
         * Add the members to the guilds.
         */
        
        for (std::list<Guild*>::iterator itr = guilds.begin();
             itr != guilds.end();
             ++itr)
        {
            std::ostringstream memberSql;
            memberSql << "select member_name from " << GUILD_MEMBERS_TBL_NAME
            << " where guild_id = '" << (*itr)->getId() << "';";
            const dal::RecordSet& memberInfo = mDb->execSql(memberSql.str());
        
            for (unsigned int j = 0; j < memberInfo.rows(); ++j)
            {
                CharacterPtr character = getCharacter(memberInfo(j,0));
                character->addGuild((*itr)->getName());
                (*itr)->addMember(character.get());
            }
        }
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what());
    }
    
    return guilds;
}
