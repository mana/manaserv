/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <time.h>

#include "account-server/storage.hpp"

#include "point.h"
#include "account-server/account.hpp"
#include "chat-server/chatchannel.hpp"
#include "chat-server/guild.hpp"
#include "chat-server/post.hpp"
#include "common/configuration.hpp"
#include "dal/dalexcept.h"
#include "dal/dataproviderfactory.h"
#include "utils/functors.h"
#include "utils/logger.h"
#include "utils/xml.hpp"

static const char *DEFAULT_ITEM_FILE = "items.xml";

// defines the supported db version
static const char *DB_VERSION_PARAMETER = "database_version";
static const char *SUPPORTED_DB_VERSION = "11";

/*
 * MySQL specificities:
 *     - TINYINT is an integer (1 byte) type defined as an extension to
 *       the SQL standard.
 *     - all integer types can have an optional (non-standard) attribute
 *       UNSIGNED (http://dev.mysql.com/doc/mysql/en/numeric-types.html)
 *
 * SQLite3 specificities:
 *     - any column (but only one for each table) with the exact type of
 *       'INTEGER PRIMARY KEY' is taken as auto-increment.
 *     - the supported data types are: NULL, INTEGER, REAL, TEXT and BLOB
 *       (http://www.sqlite.org/datatype3.html)
 *     - the size of TEXT cannot be set, it is just ignored by the engine.
 *     - IMPORTANT: foreign key constraints are not yet supported
 *       (http://www.sqlite.org/omitted.html). Included in case of future
 *       support.
 *
 * Notes:
 *     - the SQL queries will take advantage of the most appropriate data
 *       types supported by a particular database engine in order to
 *       optimize the server database size.
 *
 * TODO: Fix problem with PostgreSQL null primary key's.
 */

static const char *ACCOUNTS_TBL_NAME            =   "mana_accounts";
static const char *CHARACTERS_TBL_NAME          =   "mana_characters";
static const char *CHAR_ATTR_TBL_NAME           =   "mana_char_attr";
static const char *CHAR_SKILLS_TBL_NAME         =   "mana_char_skills";
static const char *CHAR_STATUS_EFFECTS_TBL_NAME =   "mana_char_status_effects";
static const char *CHAR_KILL_COUNT_TBL_NAME     =   "mana_char_kill_stats";
static const char *CHAR_SPECIALS_TBL_NAME       =  "mana_char_specials";
static const char *CHAR_EQUIPS_TBL_NAME         =   "mana_char_equips";
static const char *INVENTORIES_TBL_NAME         =   "mana_inventories";
static const char *ITEMS_TBL_NAME               =   "mana_items";
static const char *GUILDS_TBL_NAME              =   "mana_guilds";
static const char *GUILD_MEMBERS_TBL_NAME       =   "mana_guild_members";
static const char *QUESTS_TBL_NAME              =   "mana_quests";
static const char *WORLD_STATES_TBL_NAME        =   "mana_world_states";
static const char *POST_TBL_NAME                =   "mana_post";
static const char *POST_ATTACHMENTS_TBL_NAME    =   "mana_post_attachments";
static const char *AUCTION_TBL_NAME             =   "mana_auctions";
static const char *AUCTION_BIDS_TBL_NAME        =   "mana_auction_bids";
static const char *ONLINE_USERS_TBL_NAME        =   "mana_online_list";
static const char *TRANSACTION_TBL_NAME         =   "mana_transactions";

/**
 * Constructor.
 */
Storage::Storage()
        : mDb(dal::DataProviderFactory::createDataProvider()),
          mItemDbVersion(0)
{
}

/**
 * Destructor.
 */
Storage::~Storage()
{
    if (mDb->isConnected())
        close();

    delete mDb;
}

/**
 * Connect to the database and initialize it if necessary.
 */
void Storage::open()
{
    // Do nothing if already connected.
    if (mDb->isConnected())
        return;

    using namespace dal;

    try {
        // open a connection to the database.
        mDb->connect();

        // check database version here
        std::string dbversion = getWorldStateVar(DB_VERSION_PARAMETER);
        if (dbversion != SUPPORTED_DB_VERSION)
        {
            std::ostringstream errmsg;
            errmsg << "Database version is not supported. " <<
                "Needed version: '" << SUPPORTED_DB_VERSION <<
                "', current version: '" << dbversion << "'";
            throw errmsg.str();
        }

        // synchronize base data from xml files
        syncDatabase();

        // clean list of online users, this should be empty after restart
        std::ostringstream sql;
        sql << "DELETE FROM " << ONLINE_USERS_TBL_NAME;
        mDb->execSql(sql.str());
    }
    catch (const DbConnectionFailure& e) {
        std::ostringstream errmsg;
        errmsg << "(DALStorage::open #1) Unable to connect to the database: "
            << e.what();
        throw errmsg.str();
    }
}

/**
 * Disconnect from the database.
 */
void Storage::close()
{
    mDb->disconnect();
}

/**
 * Gets an account from a prepared SQL statement
 *
 * @return the account found
 */
Account *Storage::getAccountBySQL()
{
    try {
        const dal::RecordSet &accountInfo = mDb->processSql();

        // if the account is not even in the database then
        // we have no choice but to return nothing.
        if (accountInfo.isEmpty())
        {
            return 0;
        }

        // specialize the string_to functor to convert
        // a string to an unsigned int.
        string_to< unsigned > toUint;
        unsigned id = toUint(accountInfo(0, 0));

        // create an Account instance
        // and initialize it with information about the user.
        Account *account = new Account(id);
        account->setName(accountInfo(0, 1));
        account->setPassword(accountInfo(0, 2));
        account->setEmail(accountInfo(0, 3));
        account->setRegistrationDate(toUint(accountInfo(0, 6)));
        account->setLastLogin(toUint(accountInfo(0, 7)));

        int level = toUint(accountInfo(0, 4));
        // Check if the user is permanently banned, or temporarily banned.
        if (level == AL_BANNED
                || time(0) <= (int) toUint(accountInfo(0, 5)))
        {
            account->setLevel(AL_BANNED);
            // It is, so skip character loading.
            return account;
        }
        account->setLevel(level);

        // load the characters associated with the account.
        std::ostringstream sql;
        sql << "select id from " << CHARACTERS_TBL_NAME << " where user_id = '"
            << id << "';";
        const dal::RecordSet &charInfo = mDb->execSql(sql.str());

        if (!charInfo.isEmpty())
        {
            int size = charInfo.rows();
            Characters characters;

            LOG_DEBUG("Account "<< id << " has " << size << " character(s) in database.");

            // Two steps: it seems like multiple requests cannot be alive at the same time.
            std::vector< unsigned > characterIDs;
            for (int k = 0; k < size; ++k)
            {
                characterIDs.push_back(toUint(charInfo(k, 0)));
            }

            for (int k = 0; k < size; ++k)
            {
                if (Character *ptr = getCharacter(characterIDs[k], account))
                {
                    characters.push_back(ptr);
                }
                else
                {
                    LOG_ERROR("Failed to get character " << characterIDs[k] << " for account " << id << '.');
                }
            }

            account->setCharacters(characters);
        }

        return account;
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("DALStorage::getAccountBySQL: " << e.what());
        return 0; // TODO: Throw exception here
    }
}

/**
 * Get an account by user name.
 *
 * @param userName the owner of the account.
 * @return the account associated to the user name.
 */
Account *Storage::getAccount(const std::string &userName)
{
    std::ostringstream sql;
    sql << "SELECT * FROM " << ACCOUNTS_TBL_NAME << " WHERE username = ?";
    if (mDb->prepareSql(sql.str()))
    {
        mDb->bindValue(1, userName);
    }
    return getAccountBySQL();
}

/**
 * Get an account by ID.
 *
 * @param accountID the ID of the account.
 *
 * @return the account associated with the ID.
 */
Account *Storage::getAccount(int accountID)
{
    std::ostringstream sql;
    sql << "SELECT * FROM " << ACCOUNTS_TBL_NAME << " WHERE id = ?";
    if (mDb->prepareSql(sql.str()))
    {
        mDb->bindValue(1, accountID);
    }
    return getAccountBySQL();
}

/**
 * Gets a character from a prepared SQL statement
 *
 * @param owner the account the character is in.
 *
 * @return the character found by the query.
 */
Character *Storage::getCharacterBySQL(Account *owner)
{
    Character *character;

    // specialize the string_to functor to convert
    // a string to an unsigned int.
    string_to< unsigned > toUint;

    try {
        const dal::RecordSet &charInfo = mDb->processSql();

        // if the character is not even in the database then
        // we have no choice but to return nothing.
        if (charInfo.isEmpty())
            return 0;

        // specialize the string_to functor to convert
        // a string to an unsigned short.
        string_to< unsigned short > toUshort;
        string_to< double > toDouble;

        character = new Character(charInfo(0, 2), toUint(charInfo(0, 0)));
        character->setGender(toUshort(charInfo(0, 3)));
        character->setHairStyle(toUshort(charInfo(0, 4)));
        character->setHairColor(toUshort(charInfo(0, 5)));
        character->setLevel(toUshort(charInfo(0, 6)));
        character->setCharacterPoints(toUshort(charInfo(0, 7)));
        character->setCorrectionPoints(toUshort(charInfo(0, 8)));
        Point pos(toUshort(charInfo(0, 9)), toUshort(charInfo(0, 10)));
        character->setPosition(pos);

        int mapId = toUint(charInfo(0, 11));
        if (mapId > 0)
        {
            character->setMapId(mapId);
        }
        else
        {
            // Set character to default map and one of the default location
            // Default map is to be 1, as not found return value will be 0.
            character->setMapId(Configuration::getValue("defaultMap", 1));
        }

        /* Fill the account-related fields. Last step, as it may require a new
           SQL query. */
        if (owner)
        {
            character->setAccount(owner);
        }
        else
        {
            int id = toUint(charInfo(0, 1));
            character->setAccountID(id);
            std::ostringstream s;
            s << "select level from " << ACCOUNTS_TBL_NAME
              << " where id = '" << id << "';";
            const dal::RecordSet &levelInfo = mDb->execSql(s.str());
            character->setAccountLevel(toUint(levelInfo(0, 0)), true);
        }

        std::ostringstream s;

        /*
         * Load attributes.
         */
        s << "SELECT attr_id, attr_base, attr_mod "
          << "FROM " << CHAR_ATTR_TBL_NAME << " "
          << "WHERE char_id = " << character->getDatabaseID();

        const dal::RecordSet &attrInfo = mDb->execSql(s.str());
        if (!attrInfo.isEmpty())
        {
            const unsigned int nRows = attrInfo.rows();
            for (unsigned int row = 0; row < nRows; ++row)
            {
                unsigned int id = toUint(attrInfo(row, 0));
                character->setAttribute(id,    toDouble(attrInfo(row, 1)));
                character->setModAttribute(id, toDouble(attrInfo(row, 2)));
            }
        }

        s.clear();
        s.str("");

        // load the skills of the char from CHAR_SKILLS_TBL_NAME

        s << "select status_id, status_time FROM " << CHAR_STATUS_EFFECTS_TBL_NAME
          << " WHERE char_id = " << character->getDatabaseID();

        const dal::RecordSet &skillInfo = mDb->execSql(s.str());
        if (!skillInfo.isEmpty())
        {
            const unsigned int nRows = skillInfo.rows();
            for (unsigned int row = 0; row < nRows; row++)
            {
                character->setExperience(
                    toUint(skillInfo(row, 0)),  // skillid
                    toUint(skillInfo(row, 1))); // experience
            }
        }
        // Load the status effect
        s.clear();
        s.str("");
        s << "select status_id, status_time FROM " << CHAR_STATUS_EFFECTS_TBL_NAME
          << " WHERE char_id = " << character->getDatabaseID();
        const dal::RecordSet &statusInfo = mDb->execSql(s.str());
        if (!statusInfo.isEmpty())
        {
            const unsigned int nRows = statusInfo.rows();
            for (unsigned int row = 0; row < nRows; row++)
            {
                character->applyStatusEffect(
                    toUint(statusInfo(row, 0)), // Statusid
                    toUint(statusInfo(row, 1))); // Time
            }
        }
        // Load the kill stats
        s.clear();
        s.str("");
        s << "select monster_id, kills FROM " << CHAR_KILL_COUNT_TBL_NAME
          << " WHERE char_id = " << character->getDatabaseID();
        const dal::RecordSet &killsInfo = mDb->execSql(s.str());
        if (!killsInfo.isEmpty())
        {
            const unsigned int nRows = killsInfo.rows();
            for (unsigned int row = 0; row < nRows; row++)
            {
                character->setKillCount(
                    toUint(killsInfo(row, 0)), // MonsterID
                    toUint(killsInfo(row, 1))); // Kills
            }
        }
        // load the special status
        s.clear();
        s.str("");
        s << "select special_id FROM " << CHAR_SPECIALS_TBL_NAME
          << " WHERE char_id = " << character->getDatabaseID();
        const dal::RecordSet &specialsInfo = mDb->execSql(s.str());
        if (!specialsInfo.isEmpty())
        {
            const unsigned int nRows = specialsInfo.rows();
            for (unsigned int row = 0; row < nRows; row++)
            {
                character->giveSpecial(toUint(specialsInfo(row, 0)));
            }
        }
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::getCharacter #1) SQL query failure: " << e.what());
        return 0;
    }

    Possessions &poss = character->getPossessions();

    try
    {
        std::ostringstream sql;
        sql << " select * from " << CHAR_EQUIPS_TBL_NAME << " where owner_id = '"
            << character->getDatabaseID() << "' order by slot_type desc;";

        const dal::RecordSet &equipInfo = mDb->execSql(sql.str());
        if (!equipInfo.isEmpty())
            for (int k = 0, size = equipInfo.rows(); k < size; ++k)
                poss.equipSlots.insert(std::pair<unsigned int, unsigned int>(
                                        toUint(equipInfo(k, 3)),
                                        toUint(equipInfo(k, 2))));
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::getCharacter #1) SQL query failure: " << e.what());
        return 0;
    }

    try
    {
        std::ostringstream sql;
        sql << " select * from " << INVENTORIES_TBL_NAME << " where owner_id = '"
            << character->getDatabaseID() << "' order by slot asc;";

        const dal::RecordSet &itemInfo = mDb->execSql(sql.str());
        if (!itemInfo.isEmpty())
            for (int k = 0, size = itemInfo.rows(); k < size; ++k)
            {
                InventoryItem item;
                unsigned short slot = toUint(itemInfo(k, 2));
                item.itemId   = toUint(itemInfo(k, 3));
                item.amount   = toUint(itemInfo(k, 4));
                poss.inventory[slot] = item;
            }
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::getCharacter #2) SQL query failure: " << e.what());
        return 0;
    }

    return character;
}

/**
 * Gets a character by database ID.
 *
 * @param id the ID of the character.
 * @param owner the account the character is in.
 *
 * @return the character associated to the ID.
 */
Character *Storage::getCharacter(int id, Account *owner)
{
    std::ostringstream sql;
    sql << "SELECT * FROM " << CHARACTERS_TBL_NAME << " WHERE id = ?";
    if (mDb->prepareSql(sql.str()))
    {
        mDb->bindValue(1, id);
    }
    return getCharacterBySQL(owner);
}

/**
 * Gets a character by character name.
 *
 * @param name of the character
 *
 * @return the character associated to the name
 */
Character *Storage::getCharacter(const std::string &name)
{
    std::ostringstream sql;
    sql << "SELECT * FROM " << CHARACTERS_TBL_NAME << " WHERE name = ?";
    if (mDb->prepareSql(sql.str()))
    {
        mDb->bindValue(1, name);
    }
    return getCharacterBySQL(0);
}

/**
 * Tells if the user name already exists.
 * @return true if the user name exists.
 */
bool Storage::doesUserNameExist(const std::string &name)
{
    try {
        std::ostringstream sql;
        sql << "SELECT COUNT(username) FROM " << ACCOUNTS_TBL_NAME
            << " WHERE username = ?";

        if (mDb->prepareSql(sql.str()))
        {
            mDb->bindValue(1, name);
        }
        const dal::RecordSet &accountInfo = mDb->processSql();

        std::istringstream ssStream(accountInfo(0, 0));
        unsigned int iReturn = 1;
        ssStream >> iReturn;
        return iReturn != 0;
    } catch (const std::exception &e) {
        // TODO: throw an exception.
        LOG_ERROR("(DALStorage::doesUserNameExist) SQL query failure: " << e.what());
    }

    return true;
}

/**
 * Tells if the email address already exists.
 * @return true if the email address exists.
 */
bool Storage::doesEmailAddressExist(const std::string &email)
{
    try {
        std::ostringstream sql;
        sql << "SELECT COUNT(email) FROM " << ACCOUNTS_TBL_NAME
            << " WHERE UPPER(email) = UPPER(?)";
        if (mDb->prepareSql(sql.str()))
        {
            mDb->bindValue(1, email);
        }
        const dal::RecordSet &accountInfo = mDb->processSql();

        std::istringstream ssStream(accountInfo(0, 0));
        unsigned int iReturn = 1;
        ssStream >> iReturn;
        return iReturn != 0;
    } catch (const std::exception &e) {
        // TODO: throw an exception.
        LOG_ERROR("(DALStorage::doesEmailAddressExist) SQL query failure: " << e.what());
    }

    return true;
}

/**
 * Tells if the character's name already exists.
 * @return true if character's name exists.
 */
bool Storage::doesCharacterNameExist(const std::string& name)
{
    try {
        std::ostringstream sql;
        sql << "SELECT COUNT(name) FROM " << CHARACTERS_TBL_NAME << " WHERE name = ?";
        if (mDb->prepareSql(sql.str()))
        {
            mDb->bindValue(1, name);
        }
        const dal::RecordSet &accountInfo = mDb->processSql();

        std::istringstream ssStream(accountInfo(0, 0));
        int iReturn = 1;
        ssStream >> iReturn;
        return iReturn != 0;
    } catch (const std::exception &e) {
        // TODO: throw an exception.
        LOG_ERROR("(DALStorage::doesCharacterNameExist) SQL query failure: "
                << e.what());
    }

    return true;
}

/**
 * Updates the data for a single character, does not update the owning account
 * or the characters name. Primary usage should be storing characterdata
 * received from a game server.
 *
 * @param ptr Character to store values in the database.
 * @param startTransaction set to false if this method is called as
 *                         nested transaction.
 * @return true on success
 */
bool Storage::updateCharacter(Character *character,
                              bool startTransaction)
{
    // Update the database Character data (see CharacterData for details)
    if (startTransaction)
    {
        mDb->beginTransaction();
    }
    try
    {
        std::ostringstream sqlUpdateCharacterInfo;
        sqlUpdateCharacterInfo
            << "update "        << CHARACTERS_TBL_NAME << " "
            << "set "
            << "gender = '"     << character->getGender() << "', "
            << "hair_style = '" << character->getHairStyle() << "', "
            << "hair_color = '" << character->getHairColor() << "', "
            << "level = '"      << character->getLevel() << "', "
            << "char_pts = '"   << character->getCharacterPoints() << "', "
            << "correct_pts = '"<< character->getCorrectionPoints() << "', "
            << "x = '"          << character->getPosition().x << "', "
            << "y = '"          << character->getPosition().y << "', "
            << "map_id = '"     << character->getMapId() << "' "
            << "where id = '"   << character->getDatabaseID() << "';";

        mDb->execSql(sqlUpdateCharacterInfo.str());
    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception.
        if (startTransaction)
        {
            mDb->rollbackTransaction();
        }
        LOG_ERROR("(DALStorage::updateCharacter #1) SQL query failure: " << e.what());
        return false;
    }

    /**
     * Character attributes.
     */
    try
    {
        std::ostringstream sqlAttr;
        for (AttributeMap::const_iterator
             it = character->mAttributes.begin(),
             it_end = character->mAttributes.end();
            it != it_end;
            ++it)
            updateAttribute(character->getDatabaseID(), it->first,
                            it->second.first, it->second.second);
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        if (startTransaction)
            mDb->rollbackTransaction();
        LOG_ERROR("(DALStorage::updateCharacter #2) SQL query failure: " << e.what());
    }

    /**
     *  Character's skills
     */
    try
    {
        std::map<int, int>::const_iterator skill_it;
        for (skill_it = character->mExperience.begin();
             skill_it != character->mExperience.end(); skill_it++)
        {
            updateExperience(character->getDatabaseID(), skill_it->first, skill_it->second);
        }
    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception.
        if (startTransaction)
        {
            mDb->rollbackTransaction();
        }
        LOG_ERROR("(DALStorage::updateCharacter #3) SQL query failure: " << e.what());
        return false;
    }

    /**
     *  Character's kill count
     */
    try
    {
        std::map<int, int>::const_iterator kill_it;
        for (kill_it = character->getKillCountBegin();
             kill_it != character->getKillCountEnd(); kill_it++)
        {
            updateKillCount(character->getDatabaseID(), kill_it->first, kill_it->second);
        }
    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception.
        if (startTransaction)
        {
            mDb->rollbackTransaction();
        }
        LOG_ERROR("(DALStorage::updateCharacter #4) SQL query failure: " << e.what());
        return false;
    }
    /**
     *  Character's special actions
     */
    try
    {
        // out with the old
        std::ostringstream deleteSql("");
        std::ostringstream insertSql;
        deleteSql   << "DELETE FROM " << CHAR_SPECIALS_TBL_NAME
                    << " WHERE char_id='" << character->getDatabaseID() << "';";
        mDb->execSql(deleteSql.str());
        // in with the new
        std::map<int, Special*>::const_iterator special_it;
        for (special_it = character->getSpecialBegin();
             special_it != character->getSpecialEnd(); special_it++)
        {
            insertSql.str("");
            insertSql   << "INSERT INTO " << CHAR_SPECIALS_TBL_NAME
                        << " (char_id, special_id) VALUES ("
                        << " '" << character->getDatabaseID() << "',"
                        << " '" << special_it->first << "');";
            mDb->execSql(insertSql.str());
        }
    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception.
        if (startTransaction)
        {
            mDb->rollbackTransaction();
        }
        LOG_ERROR("(DALStorage::updateCharacter #5) SQL query failure: " << e.what());
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
        if (startTransaction)
        {
            mDb->rollbackTransaction();
        }
        LOG_ERROR("(DALStorage::updateCharacter #5) SQL query failure: " << e.what());
        return false;
    }

    // Insert the new inventory data
    try
    {
        std::ostringstream sql;

        sql << "insert into " << CHAR_EQUIPS_TBL_NAME
            << " (owner_id, slot_type, inventory_slot) values ("
            << character->getDatabaseID() << ", ";
        std::string base = sql.str();

        const Possessions &poss = character->getPossessions();
        for (EquipData::const_iterator it = poss.equipSlots.begin(),
             it_end = poss.equipSlots.end();
             it != it_end;
             ++it)
        {
                sql.str("");
                sql << base << it->first << ", " << it->second << ");";
                mDb->execSql(sql.str());
        }

        sql.str("");

        sql << "insert into " << INVENTORIES_TBL_NAME
            << " (owner_id, slot, class_id, amount) values ("
            << character->getDatabaseID() << ", ";
        base = sql.str();

        for (InventoryData::const_iterator j = poss.inventory.begin(),
             j_end = poss.inventory.end(); j != j_end; ++j)
        {
            sql.str("");
            unsigned short slot = j->first;
            unsigned int   itemId = j->second.itemId;
            unsigned int   amount = j->second.amount;
            assert(itemId);
            sql << base << slot << ", " << itemId << ", " << amount << ");";
            mDb->execSql(sql.str());
        }

    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception.
        if (startTransaction)
        {
            mDb->rollbackTransaction();
        }
        LOG_ERROR("(DALStorage::updateCharacter #6) SQL query failure: " << e.what());
        return false;
    }

    /**
     * Update char status effects
     */
    try
    {
        // Delete the old status effects first
        std::ostringstream sql;

        sql << "delete from " << CHAR_STATUS_EFFECTS_TBL_NAME
            << " where char_id = '" << character->getDatabaseID() << "';";

         mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception.
        if (startTransaction)
        {
            mDb->rollbackTransaction();
        }
        LOG_ERROR("(DALStorage::updateCharacter #7) SQL query failure: " << e.what());
        return false;
    }
    try
    {
        std::map<int, int>::const_iterator status_it;
        for (status_it = character->getStatusEffectBegin();
             status_it != character->getStatusEffectEnd(); status_it++)
        {
            insertStatusEffect(character->getDatabaseID(), status_it->first, status_it->second);
        }
    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception
        if (startTransaction)
        {
            mDb->rollbackTransaction();
        }
        LOG_ERROR("(DALStorage::updateCharacter #8) SQL query failure: " << e.what());
        return false;
    }
    if (startTransaction)
    {
        mDb->commitTransaction();
    }
    return true;
}

/**
 * Save changes of a skill to the database permanently.
 *
 * @param character Character thats skill has changed.
 * @param skill_id Identifier of the changed skill.
 *
 * @exception dbl::DbSqlQueryExecFailure.
 * @deprecated Use DALStorage::updateExperience instead!!!
*/
void Storage::flushSkill(const Character *character, int skill_id)
{
    updateExperience(character->getDatabaseID(), skill_id,
        character->getExperience(skill_id));
}

/**
 * Add an account to the database.
 * @param account the new account.
 */
void Storage::addAccount(Account *account)
{
    assert(account->getCharacters().size() == 0);

    using namespace dal;

    mDb->beginTransaction();
    try
    {
        // insert the account
        std::ostringstream sql;
        sql << "insert into " << ACCOUNTS_TBL_NAME
             << " (username, password, email, level, banned, registration, lastlogin)"
             << " VALUES (?, ?, ?, "
             << account->getLevel() << ", 0, "
             << account->getRegistrationDate() << ", "
             << account->getLastLogin() << ");";

        if (mDb->prepareSql(sql.str()))
        {
            mDb->bindValue(1, account->getName());
            mDb->bindValue(2, account->getPassword());
            mDb->bindValue(3, account->getEmail());
        }

        mDb->processSql();
        account->setID(mDb->getLastId());

        mDb->commitTransaction();
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("Error in DALStorage::addAccount: " << e.what());
        mDb->rollbackTransaction();
    }
}

/**
 * Update an account from the database.
 */
void Storage::flush(Account *account)
{
    assert(account->getID() >= 0);

    using namespace dal;

    mDb->beginTransaction();
    try
    {
        // update the account
        std::ostringstream sqlUpdateAccountTable;
        sqlUpdateAccountTable
             << "update " << ACCOUNTS_TBL_NAME
             << " set username = '" << account->getName() << "', "
             << "password = '" << account->getPassword() << "', "
             << "email = '" << account->getEmail() << "', "
             << "level = '" << account->getLevel() << "', "
             << "lastlogin = '" << account->getLastLogin() << "' "
             << "where id = '" << account->getID() << "';";
        mDb->execSql(sqlUpdateAccountTable.str());

        // get the list of characters that belong to this account.
        Characters &characters = account->getCharacters();

        // insert or update the characters.
        for (Characters::const_iterator it = characters.begin(),
             it_end = characters.end(); it != it_end; ++it)
        {
            if ((*it)->getDatabaseID() >= 0)
            {
                /* 2nd. parameter false means: don't start a transaction in
                   the updateCharacter method, cause we did this already a few
                   lines above */
                updateCharacter(*it, false);
            }
            else
            {
                std::ostringstream sqlInsertCharactersTable;
                // insert the character
                // This assumes that the characters name has been checked for
                // uniqueness
                sqlInsertCharactersTable
                     << "insert into " << CHARACTERS_TBL_NAME
                     << " (user_id, name, gender, hair_style, hair_color, level, char_pts, correct_pts,"
                     << " x, y, map_id) values ("
                     << account->getID() << ", \""
                     << (*it)->getName() << "\", "
                     << (*it)->getGender() << ", "
                     << (int)(*it)->getHairStyle() << ", "
                     << (int)(*it)->getHairColor() << ", "
                     << (int)(*it)->getLevel() << ", "
                     << (int)(*it)->getCharacterPoints() << ", "
                     << (int)(*it)->getCorrectionPoints() << ", "
                     << (*it)->getPosition().x << ", "
                     << (*it)->getPosition().y << ", "
                     << (*it)->getMapId()
                     << ");";

                mDb->execSql(sqlInsertCharactersTable.str());

                // Update the character ID.
                (*it)->setDatabaseID(mDb->getLastId());

                // Update all attributes.
                std::map<unsigned int, std::pair<double, double> >::const_iterator
                        attr_it, attr_end;
                for (attr_it =  (*it)->mAttributes.begin(),
                     attr_end = (*it)->mAttributes.end();
                    attr_it != attr_end;
                    ++attr_it)
                    updateAttribute((*it)->getDatabaseID(), attr_it->first,
                                    attr_it->second.first,
                                    attr_it->second.second);

                // update the characters skill
                std::map<int, int>::const_iterator skill_it;
                for (skill_it = (*it)->mExperience.begin();
                     skill_it != (*it)->mExperience.end();
                     skill_it++)
                    updateExperience((*it)->getDatabaseID(), skill_it->first, skill_it->second);
            }
        } //

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
                delCharacter(charId, false);
            }
        }

        mDb->commitTransaction();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("ERROR in DALStorage::flush: " << e.what());
        mDb->rollbackTransaction();
    }
}

/**
 * Delete an account and its associated data from the database.
 *
 * @param account the account to delete.
 */
void Storage::delAccount(Account *account)
{
    account->setCharacters(Characters());
    flush(account);

    // delete the account.
    std::ostringstream sql;
    sql << "delete from " << ACCOUNTS_TBL_NAME
        << " where id = '" << account->getID() << "';";
    mDb->execSql(sql.str());
}

/**
 * Update the date and time of the last login.
 *
 * @param account the account that recently logged in.
 */
void Storage::updateLastLogin(const Account *account)
{
    std::ostringstream sql;
    sql << "UPDATE " << ACCOUNTS_TBL_NAME
        << "   SET lastlogin = '" << account->getLastLogin() << "'"
        << " WHERE id = '" << account->getID() << "';";
    mDb->execSql(sql.str());
}

/**
 * Write a modification message about Character points to the database.
 *
 * @param CharId      ID of the character
 * @param CharPoints  Number of character points left for the character
 * @param CorrPoints  Number of correction points left for the character
 */
void Storage::updateCharacterPoints(int charId,
                                    int charPoints, int corrPoints)
{
    try
    {
        std::ostringstream sql;
        sql << "UPDATE " << CHARACTERS_TBL_NAME
            << " SET char_pts = " << charPoints << ", "
            << " correct_pts = " << corrPoints << ", "
            << " WHERE id = " << charId;

        mDb->execSql(sql.str());
    }
    catch (dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("DALStorage::updateCharacterPoints: " << e.what());
        throw;
    }

}

/**
 * Write a modification message about character skills to the database.
 * @param CharId      ID of the character
 * @param SkillId     ID of the skill
 * @param SkillValue  new skill points
 */
void Storage::updateExperience(int charId, int skillId, int skillValue)
{
    try
    {
        // if experience has decreased to 0 we don't store it anymore,
        // its the default
        if (skillValue == 0)
        {
            std::ostringstream sql;
            sql << "DELETE FROM " << CHAR_SKILLS_TBL_NAME
                << " WHERE char_id = " << charId
                << " AND skill_id = " << skillId;
            mDb->execSql(sql.str());
            return;
        }

        // try to update the skill
        std::ostringstream sql;
        sql << "UPDATE " << CHAR_SKILLS_TBL_NAME
            << " SET skill_exp = " << skillValue
            << " WHERE char_id = " << charId
            << " AND skill_id = " << skillId;
        mDb->execSql(sql.str());

        // check if the update has modified a row
        if (mDb->getModifiedRows() > 0)
        {
            return;
        }

        sql.clear();
        sql.str("");
        sql << "INSERT INTO " << CHAR_SKILLS_TBL_NAME << " "
            << "(char_id, skill_id, skill_exp) VALUES ( "
            << charId << ", "
            << skillId << ", "
            << skillValue << ")";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("DALStorage::updateExperience: " << e.what());
        throw;
    }
}

/**
 * Write a modification message about character attributes to the database.
 * @param charId    The Id of the character
 * @param attrId    The Id of the attribute
 * @param base      The base value of the attribute for this character
 * @param mod       The cached modified value for this character.
 */

void Storage::updateAttribute(int charId, unsigned int attrId,
                              double base, double mod)
{
    try {
        std::ostringstream sql;
        sql << "UPDATE "       << CHAR_ATTR_TBL_NAME         << " "
            << "SET "
            << "attr_base = '" << base                       << "', "
            << "attr_mod  = '" << mod                        << "' "
            << "WHERE "
            << "char_id   = '" << charId                     << "' "
            << "AND "
            << "attr_id   = '" << attrId                     << "';";
        mDb->execSql(sql.str());
        // If this has modified a row, we're done, it updated sucessfully.
        if (mDb->getModifiedRows() > 0)
            return;
        // If it did not change anything, then the record didn't previously exist.
        // Create it.
        sql.clear();
        sql.str("");
        sql << "INSERT INTO " << CHAR_ATTR_TBL_NAME         << " "
            << "(char_id, attr_id, attr_base, attr_mod) VALUES ( "
            << charId                                       << ", "
            << attrId                                       << ", "
            << base                                         << ", "
            << mod                                          << ")";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("DALStorage::updateAttribute: " << e.what());
        throw;
    }
}

/**
 * Write a modification message about character skills to the database.
 * @param CharId      ID of the character
 * @param monsterId   ID of the monster type
 * @param kills       new amount of kills
 */
void Storage::updateKillCount(int charId, int monsterId, int kills)
{
    try
    {
        // try to update the kill count
        std::ostringstream sql;
        sql << "UPDATE " << CHAR_KILL_COUNT_TBL_NAME
            << " SET kills = " << kills
            << " WHERE char_id = " << charId
            << " AND monster_id = " << monsterId;
        mDb->execSql(sql.str());

        // check if the update has modified a row
        if (mDb->getModifiedRows() > 0)
        {
            return;
        }

        sql.clear();
        sql.str("");
        sql << "INSERT INTO " << CHAR_KILL_COUNT_TBL_NAME << " "
            << "(char_id, monster_id, kills) VALUES ( "
            << charId << ", "
            << monsterId << ", "
            << kills << ")";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("DALStorage::updateKillCount: " << e.what());
        throw;
    }
}

/**
 * Inserts a record about a status effect into the database
 * @param charId    ID of the character in the database
 * @param statusId  ID of the status effect
 * @param time      Time left on the status effect
 */
void Storage::insertStatusEffect(int charId, int statusId, int time)
{
    try
    {
        std::ostringstream sql;

        sql << "insert into " << CHAR_STATUS_EFFECTS_TBL_NAME
            << " (char_id, status_id, status_time) VALUES ( "
            << charId << ", "
            << statusId << ", "
            << time << ")";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("DALStorage::insertStatusEffect: " << e.what());
        throw;
    }
}


/**
 * Add a new guild.
 */
void Storage::addGuild(Guild *guild)
{
    std::ostringstream insertSql;
    insertSql << "insert into " << GUILDS_TBL_NAME
        << " (name) VALUES (?)";
    if (mDb->prepareSql(insertSql.str()))
    {
        mDb->bindValue(1, guild->getName());
    }
    mDb->processSql();

    std::ostringstream selectSql;
    selectSql << "SELECT id FROM " << GUILDS_TBL_NAME
        << " WHERE name = ?";

    if (mDb->prepareSql(selectSql.str()))
    {
        mDb->bindValue(1, guild->getName());
    }
    const dal::RecordSet& guildInfo = mDb->processSql();

    string_to<unsigned int> toUint;
    unsigned id = toUint(guildInfo(0, 0));
    guild->setId(id);
}

/**
 * Delete a guild.
 */
void Storage::removeGuild(Guild *guild)
{
    std::ostringstream sql;
    sql << "delete from " << GUILDS_TBL_NAME
        << " where id = '"
        << guild->getId() << "';";
    mDb->execSql(sql.str());
}

/**
 * Add member to guild.
 */
void Storage::addGuildMember(int guildId, int memberId)
{
    std::ostringstream sql;

    try
    {
        sql << "insert into " << GUILD_MEMBERS_TBL_NAME
        << " (guild_id, member_id, rights)"
        << " values ("
        << guildId << ", \""
        << memberId << "\", "
        << 0 << ");";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what());
    }
}

/**
 * Remove member from guild.
 */
void Storage::removeGuildMember(int guildId, int memberId)
{
    std::ostringstream sql;

    try
    {
        sql << "delete from " << GUILD_MEMBERS_TBL_NAME
        << " where member_id = \""
        << memberId << "\" and guild_id = '"
        << guildId << "';";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what());
    }
}

/**
 * Save guild member rights.
 */
void Storage::setMemberRights(int guildId, int memberId, int rights)
{
    std::ostringstream sql;

    try
    {
        sql << "update " << GUILD_MEMBERS_TBL_NAME
        << " set rights = '" << rights << "'"
        << " where member_id = \""
        << memberId << "\";";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure& e)
    {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what());
    }
}

/**
 * Get the list of guilds.
 * @return a list of guilds
 */
std::list<Guild*> Storage::getGuildList()
{
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
        if (guildInfo.isEmpty())
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
        string_to< unsigned > toUint;

        /**
         * Add the members to the guilds.
         */
        for (std::list<Guild*>::iterator itr = guilds.begin();
             itr != guilds.end();
             ++itr)
        {
            std::ostringstream memberSql;
            memberSql << "select member_id, rights from " << GUILD_MEMBERS_TBL_NAME
            << " where guild_id = '" << (*itr)->getId() << "';";
            const dal::RecordSet& memberInfo = mDb->execSql(memberSql.str());

            std::list<std::pair<int, int> > members;
            for (unsigned int j = 0; j < memberInfo.rows(); ++j)
            {
                members.push_back(std::pair<int, int>(toUint(memberInfo(j, 0)), toUint(memberInfo(j, 1))));
            }

            for (std::list<std::pair<int, int> >::const_iterator i = members.begin();
                 i != members.end();
                 ++i)
            {
                Character *character = getCharacter((*i).first, 0);
                if (character)
                {
                    character->addGuild((*itr)->getName());
                    (*itr)->addMember(character->getDatabaseID(), (*i).second);
                }
            }
        }
    }
    catch (const dal::DbSqlQueryExecFailure& e) {
        // TODO: throw an exception.
        LOG_ERROR("SQL query failure: " << e.what());
    }

    return guilds;
}

/**
 * Gets the value of a quest variable.
 */
std::string Storage::getQuestVar(int id, const std::string &name)
{
    try
    {
        std::ostringstream query;
        query << "select value from " << QUESTS_TBL_NAME
                << " WHERE owner_id = ? AND name = ?";
        if (mDb->prepareSql(query.str()))
        {
            mDb->bindValue(1, id);
            mDb->bindValue(2, name);
        }
        const dal::RecordSet &info = mDb->processSql();

        if (!info.isEmpty()) return info(0, 0);
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::getQuestVar) SQL query failure: " << e.what());
    }

    return std::string();
}

/**
 * Gets the string value of a map specific world state variable.
 *
 * @param name Name of the requested world-state variable.
 * @param map_id Id of the specific map.
 */
std::string Storage::getWorldStateVar(const std::string &name, int map_id)
{
    try
    {
        std::ostringstream query;
        query << "SELECT value "
              << "  FROM " << WORLD_STATES_TBL_NAME
              << " WHERE state_name = '" << name << "'";

        // add map filter if map_id is given
        if (map_id >= 0)
        {
            query << "  AND map_id = '" << map_id << "'";
        }

        query << ";";
        const dal::RecordSet &info = mDb->execSql(query.str());

        if (!info.isEmpty()) return info(0, 0);
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::getWorldStateVar) SQL query failure: " << e.what());
    }

    return std::string();
}

/**
 * Sets the value of a world state variable.
 *
 * @param name Name of the world-state vairable.
 * @param value New value of the world-state variable.
 */
void Storage::setWorldStateVar(const std::string &name,
                               const std::string &value)
{
    return setWorldStateVar(name, -1, value);
}

/**
 * Sets the value of a world state variable of a specific map.
 *
 * @param name Name of the world-state vairable.
 * @param mapId ID of the specific map
 * @param value New value of the world-state variable.
 */
void Storage::setWorldStateVar(const std::string &name,
                               int mapId,
                               const std::string &value)
{
    try
    {
        // set the value to empty means: delete the variable
        if (value.empty())
        {
            std::ostringstream deleteStateVar;
            deleteStateVar << "DELETE FROM " << WORLD_STATES_TBL_NAME
                           << " WHERE state_name = '" << name << "'";
            if (mapId >= 0)
            {
                deleteStateVar << " AND map_id = '" << mapId << "'";
            }
            deleteStateVar << ";";
            mDb->execSql(deleteStateVar.str());
            return;
        }

        // try to update the variable in the database
        std::ostringstream updateStateVar;
        updateStateVar << "UPDATE " << WORLD_STATES_TBL_NAME
                       << "   SET value = '" << value << "', "
                       << "       moddate = '" << time(0) << "' "
                       << " WHERE state_name = '" << name << "'";

        if (mapId >= 0)
        {
            updateStateVar << "   AND map_id = '" << mapId << "'";
        }
        updateStateVar << ";";
        mDb->execSql(updateStateVar.str());

        // if we updated a row, were finished here
        if (mDb->getModifiedRows() >= 1)
        {
            return;
        }

        // otherwise we have to add the new variable
        std::ostringstream insertStateVar;
        insertStateVar << "INSERT INTO " << WORLD_STATES_TBL_NAME
                       << " (state_name, map_id, value , moddate) VALUES ("
                       << "'" << name << "', ";
        if (mapId >= 0)
        {
            insertStateVar << "'" << mapId << "', ";
        }
        else
        {
            insertStateVar << "0 , ";
        }
        insertStateVar << "'" << value << "', "
                       << "'" << time(0) << "');";
        mDb->execSql(insertStateVar.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::setWorldStateVar) SQL query failure: " << e.what());
    }
}

/**
 * Sets the value of a quest variable.
 */
void Storage::setQuestVar(int id, const std::string &name,
                          const std::string &value)
{
    try
    {
        std::ostringstream query1;
        query1 << "delete from " << QUESTS_TBL_NAME
               << " where owner_id = '" << id << "' and name = '"
               << name << "';";
        mDb->execSql(query1.str());

        if (value.empty()) return;

        std::ostringstream query2;
        query2 << "insert into " << QUESTS_TBL_NAME
               << " (owner_id, name, value) values ('"
               << id << "', '" << name << "', '" << value << "');";
        mDb->execSql(query2.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::setQuestVar) SQL query failure: " << e.what());
    }
}

/**
 * Sets a ban on an account (hence on all its characters).
 *
 * @param id character identifier.
 * @param duration duration in minutes.
 */
void Storage::banCharacter(int id, int duration)
{
    try
    {
        std::ostringstream query;
        query << "select user_id from " << CHARACTERS_TBL_NAME
              << " where id = '" << id << "';";
        const dal::RecordSet &info = mDb->execSql(query.str());
        if (info.isEmpty())
        {
            LOG_ERROR("Tried to ban an unknown user.");
            return;
        }

        std::ostringstream sql;
        sql << "update " << ACCOUNTS_TBL_NAME
            << " set level = '" << AL_BANNED << "', banned = '"
            << time(0) + duration * 60
            << "' where id = '" << info(0, 0) << "';";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::banAccount) SQL query failure: " << e.what());
    }
}

/**
 * Delete a character in the database.
 *
 * @param charId character identifier.
 * @param startTransaction indicates wheter the function should run in
 *        its own transaction or is called inline of another transaction
 */
void Storage::delCharacter(int charId, bool startTransaction = true) const
{
    if (startTransaction)
        mDb->beginTransaction();
    try
    {
        std::ostringstream sql;

        // delete the inventory of the character
        sql << "DELETE FROM " << INVENTORIES_TBL_NAME
            << " WHERE owner_id = '" << charId << "';";
        mDb->execSql(sql.str());

        // delete the skills of the character
        sql.clear();
        sql.str("");
        sql << "DELETE FROM " << CHAR_SKILLS_TBL_NAME
            << " WHERE char_id = '" << charId << "';";
        mDb->execSql(sql.str());

        // delete from the quests table
        sql.clear();
        sql.str("");
        sql << "DELETE FROM " << QUESTS_TBL_NAME
            << " WHERE owner_id = '" << charId << "';";
        mDb->execSql(sql.str());

        // delete from the guilds table
        sql.clear();
        sql.str("");
        sql << "DELETE FROM " << GUILD_MEMBERS_TBL_NAME
            << " WHERE member_id = '" << charId << "';";
        mDb->execSql(sql.str());

        // delete auctions of the character
        sql.clear();
        sql.str("");
        sql << "DELETE FROM " << AUCTION_TBL_NAME
            << " WHERE char_id = '" << charId << "';";
        mDb->execSql(sql.str());

        // delete bids made on auctions made by the character
        sql.clear();
        sql.str("");
        sql << "DELETE FROM " << AUCTION_BIDS_TBL_NAME
            << " WHERE char_id = '" << charId << "';";
        mDb->execSql(sql.str());

        // now delete the character itself.
        sql.clear();
        sql.str("");
        sql << "DELETE FROM " << CHARACTERS_TBL_NAME
            << " WHERE id = '" << charId << "';";
        mDb->execSql(sql.str());

        if (startTransaction)
            mDb->commitTransaction();
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        if (startTransaction)
            mDb->rollbackTransaction();
        LOG_ERROR("(DALStorage::delCharacter) SQL query failure: " << e.what());
    }
}

/**
 * Delete a character in the database. The object itself is not touched
 * by this function!
 *
 * @param character character object.
 * @param startTransaction indicates wheter the function should run in
 *        its own transaction or is called inline of another transaction
 */
void Storage::delCharacter(Character *character,
                           bool startTransaction = true) const
{
    delCharacter(character->getDatabaseID(), startTransaction);
}

/**
 * Removes expired bans from accounts
 */
void Storage::checkBannedAccounts()
{
    try
    {
        // update expired bans
        std::ostringstream sql;
        sql << "update " << ACCOUNTS_TBL_NAME
        << " set level = " << AL_PLAYER << ", banned = 0"
        << " where level = " << AL_BANNED
        << " AND banned <= " << time(0) << ";";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::checkBannedAccounts) SQL query failure: " << e.what());
    }
}

/**
 * Set the level on an account.
 *
 * @param id The id of the account
 * @param level The level to set for the account
 */
void Storage::setAccountLevel(int id, int level)
{
    try
    {
        std::ostringstream sql;
        sql << "update " << ACCOUNTS_TBL_NAME
        << " set level = " << level
        << " where id = " << id << ";";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::setAccountLevel) SQL query failure: " << e.what());
    }
}

/**
 * Set the level on a character.
 *
 * @param id The id of the character
 * @param level The level to set for the character
 */
void Storage::setPlayerLevel(int id, int level)
{
    try
    {
        std::ostringstream sql;
        sql << "update " << CHARACTERS_TBL_NAME
        << " set level = " << level
        << " where id = " << id << ";";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::setPlayerLevel) SQL query failure: " << e.what());
    }
}

/**
 * Store letter.
 *
 * @param letter The letter to store
 */
void Storage::storeLetter(Letter *letter)
{
    std::ostringstream sql;
    if (letter->getId() == 0)
    {
        // the letter was never saved before
        sql << "INSERT INTO " << POST_TBL_NAME << " VALUES ( "
            << "NULL, "
            << letter->getSender()->getDatabaseID() << ", "
            << letter->getReceiver()->getDatabaseID() << ", "
            << letter->getExpiry() << ", "
            << time(0) << ", "
            << "?)";
        if (mDb->prepareSql(sql.str()))
        {
            mDb->bindValue(1, letter->getContents());
        }

        mDb->processSql();
        letter->setId(mDb->getLastId());

        // TODO: store attachments in the database

        return;
    }
    else
    {
        // the letter has a unique id, update the record in the db
        sql << "UPDATE " << POST_TBL_NAME
            << "   SET sender_id       = '" << letter->getSender()->getDatabaseID() << "', "
            << "       receiver_id     = '" << letter->getReceiver()->getDatabaseID() << "', "
            << "       letter_type     = '" << letter->getType() << "', "
            << "       expiration_date = '" << letter->getExpiry() << "', "
            << "       sending_date    = '" << time(0) << "', "
            << "       letter_text = ? "
            << " WHERE letter_id       = '" << letter->getId() << "'";

        if (mDb->prepareSql(sql.str()))
        {
            mDb->bindValue(1, letter->getContents());
        }
        mDb->processSql();

        if (mDb->getModifiedRows() == 0)
        {
            // this should never happen...
            LOG_ERROR("(DALStorage::storePost) trying to update nonexistant letter");
            throw "(DALStorage::storePost) trying to update nonexistant letter";
        }

        // TODO: update attachments in the database
    }
}

/**
 * Retrieve post
 *
 * @param playerId The id of the character requesting his post
 */
Post *Storage::getStoredPost(int playerId)
{
    Post *p = new Post();
    // specialize the string_to functor to convert
    // a string to an unsigned int.
    string_to< unsigned > toUint;

    std::ostringstream sql;
    sql << "SELECT * FROM " << POST_TBL_NAME
        << " WHERE receiver_id = " << playerId;

    const dal::RecordSet &post = mDb->execSql(sql.str());

    if (post.isEmpty())
    {
        // there is no post waiting for the character
        return p;
    }

    for (unsigned int i = 0; i < post.rows(); i++ )
    {
        // load sender and receiver
        Character *sender = getCharacter(toUint(post(i, 1)), 0);
        Character *receiver = getCharacter(toUint(post(i, 2)), 0);

        Letter *letter = new Letter(toUint( post(0,3) ), sender, receiver);

        letter->setId( toUint(post(0, 0)) );
        letter->setExpiry( toUint(post(0, 4)) );
        letter->addText( post(0, 6) );

        // TODO: load attachments per letter from POST_ATTACHMENTS_TBL_NAME
        // needs redesign of struct ItemInventroy

        p->addLetter(letter);
    }

    return p;
}

/**
 * Delete a letter from the database.
 * @param letter The letter to delete.
 */
void Storage::deletePost(Letter *letter)
{
    mDb->beginTransaction();

    try
    {
        std::ostringstream sql;

        // first delete all attachments of the letter
        // this could leave "dead" items in the item_instances table
        sql << "DELETE FROM " << POST_ATTACHMENTS_TBL_NAME
            << " WHERE letter_id = " << letter->getId();
        mDb->execSql(sql.str());

        // delete the letter itself
        sql.clear();
        sql.str("");
        sql << "DELETE FROM " << POST_TBL_NAME
            << " WHERE letter_id = " << letter->getId();
        mDb->execSql(sql.str());

        mDb->commitTransaction();
        letter->setId(0);
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        mDb->rollbackTransaction();
        LOG_ERROR("(DALStorage::deletePost) SQL query failure: " << e.what());
    }
}

/**
 * Synchronizes the base data in the connected SQL database with the xml
 * files like items.xml.
 * This method is called once after initialization of DALStorage.
 * Probably this function should be called if a gm requests an online
 * reload of the xml files to load new items or monsters without server
 * restart.
 */
void Storage::syncDatabase()
{
    XML::Document doc(DEFAULT_ITEM_FILE);
    xmlNodePtr rootNode = doc.rootNode();

    if (!rootNode || !xmlStrEqual(rootNode->name, BAD_CAST "items"))
    {
        LOG_ERROR("Item Manager: Error while loading item database (items.xml)!");
        return;
    }

    mDb->beginTransaction();
    int itmCount = 0;
    for_each_xml_child_node(node, rootNode)
    {
        // Try to load the version of the item database. The version is defined
        // as subversion tag embedded as XML attribute. So every modification
        // to the items.xml file will increase the revision automatically.
        if (xmlStrEqual(node->name, BAD_CAST "version"))
        {
            std::string revision = XML::getProperty(node, "revision", std::string());
            mItemDbVersion = atoi(revision.c_str());
            LOG_INFO("Loading item database version " << mItemDbVersion);
        }

        if (!xmlStrEqual(node->name, BAD_CAST "item"))
            continue;

        if (xmlStrEqual(node->name, BAD_CAST "item"))
        {
            int id = XML::getProperty(node, "id", 0);
            if (id < 500)
                continue;

            int weight = XML::getProperty(node, "weight", 0);
            std::string type = XML::getProperty(node, "type", "");
            std::string name = XML::getProperty(node, "name", "");
            std::string desc = XML::getProperty(node, "description", "");
            std::string eff  = XML::getProperty(node, "effect", "");
            std::string image = XML::getProperty(node, "image", "");
            std::string dye("");

            // split image name and dye string
            size_t pipe = image.find("|");
            if (pipe != std::string::npos)
            {
                dye = image.substr(pipe + 1);
                image = image.substr(0, pipe);
            }

            try
            {
                std::ostringstream sql;
                sql << "UPDATE " << ITEMS_TBL_NAME
                    << " SET name = ?, "
                    << "     description = ?, "
                    << "     image = '" << image << "', "
                    << "     weight = " << weight << ", "
                    << "     itemtype = '" << type << "', "
                    << "     effect = ?, "
                    << "     dyestring = '" << dye << "' "
                    << " WHERE id = " << id;

                if (mDb->prepareSql(sql.str()))
                {
                    mDb->bindValue(1, name);
                    mDb->bindValue(2, desc);
                    mDb->bindValue(3, eff);
                }
                mDb->processSql();
                if (mDb->getModifiedRows() == 0)
                {
                    sql.clear();
                    sql.str("");
                    sql << "INSERT INTO " << ITEMS_TBL_NAME
                        << "  VALUES ( " << id << ", ?, ?, '"
                        << image << "', " << weight << ", '"
                        << type << "', ?, '" << dye << "' )";
                    if (mDb->prepareSql(sql.str()))
                    {
                        mDb->bindValue(1, name);
                        mDb->bindValue(2, desc);
                        mDb->bindValue(3, eff);
                    }
                    mDb->processSql();
                }
                itmCount++;
            }
            catch (const dal::DbSqlQueryExecFailure &e)
            {
                LOG_ERROR("(DALStorage::SyncDatabase) SQL query failure: " << e.what());
            }
        }
    }

    mDb->commitTransaction();
}

/**
 * Sets the status of a character to online (true) or offline (false).
 *
 * @param charId Id of the character.
 * @param online True to mark the character as being online.
 */
void Storage::setOnlineStatus(int charId, bool online)
{
    try
    {
        std::ostringstream sql;
        if (online)
        {
            // first we try to update the online status. this prevents errors
            // in case we get the online status twice
            sql << "SELECT COUNT(*) FROM " << ONLINE_USERS_TBL_NAME
                << " WHERE char_id = " << charId;
            const std::string res = mDb->execSql(sql.str())(0, 0);

            if (res != "0")
                return;

            sql.clear();
            sql.str("");
            sql << "INSERT INTO " << ONLINE_USERS_TBL_NAME
                << " VALUES (" << charId << ", " << time(0) << ")";
            mDb->execSql(sql.str());
        }
        else
        {
            sql << "DELETE FROM " << ONLINE_USERS_TBL_NAME
                << " WHERE char_id = " << charId;
            mDb->execSql(sql.str());
        }


    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::setOnlineStatus) SQL query failure: " << e.what());
    }
}

/**
 * Store a transaction.
 */
void Storage::addTransaction(const Transaction &trans)
{
    try
    {
        std::stringstream sql;
        sql << "INSERT INTO " << TRANSACTION_TBL_NAME
            << " VALUES (NULL, " << trans.mCharacterId << ", "
            << trans.mAction << ", "
            << "?, "
            << time(0) << ")";
        if (mDb->prepareSql(sql.str()))
        {
            mDb->bindValue(1, trans.mMessage);
        }
        mDb->processSql();
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::addTransaction) SQL query failure: " << e.what());
    }
}

/**
 * Retrieve the last \num transactions that were stored.
 */
std::vector<Transaction> Storage::getTransactions(unsigned int num)
{
    std::vector<Transaction> transactions;
    string_to<unsigned int> toUint;

    try
    {
        std::stringstream sql;
        sql << "SELECT * FROM " << TRANSACTION_TBL_NAME;
        const dal::RecordSet &rec = mDb->execSql(sql.str());

        int size = rec.rows();
        int start = size - num;
        // Get the last <num> records and store them in transactions
        for (int i = start; i < size; ++i)
        {
            Transaction trans;
            trans.mCharacterId = toUint(rec(i, 1));
            trans.mAction = toUint(rec(i, 2));
            trans.mMessage = rec(i, 3);
            transactions.push_back(trans);
        }
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::getTransactions) SQL query failure: " << e.what());
    }

    return transactions;
}

/**
 * Retrieve all transactions since the given \a date.
 */
std::vector<Transaction> Storage::getTransactions(time_t date)
{
    std::vector<Transaction> transactions;
    string_to<unsigned int> toUint;

    try
    {
        std::stringstream sql;
        sql << "SELECT * FROM " << TRANSACTION_TBL_NAME << " WHERE time > "
            << date;
        const dal::RecordSet &rec = mDb->execSql(sql.str());

        for (unsigned int i = 0; i < rec.rows(); ++i)
        {
            Transaction trans;
            trans.mCharacterId = toUint(rec(i, 1));
            trans.mAction = toUint(rec(i, 2));
            trans.mMessage = rec(i, 3);
            transactions.push_back(trans);
        }
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::getTransactions) SQL query failure: " << e.what());
    }

    return transactions;
}
