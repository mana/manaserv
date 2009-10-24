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
 */

#include <cassert>
#include <time.h>

#include "account-server/dalstorage.hpp"

#include "point.h"
#include "account-server/account.hpp"
#include "account-server/dalstoragesql.hpp"
#include "chat-server/chatchannel.hpp"
#include "chat-server/guild.hpp"
#include "chat-server/post.hpp"
#include "common/configuration.hpp"
#include "dal/dalexcept.h"
#include "dal/dataproviderfactory.h"
#include "utils/functors.h"
#include "utils/logger.h"
#include "utils/xml.hpp"

// TODO: make data/items.xml a constant or read it from config file
#define DEFAULT_ITEM_FILE       "data/items.xml"

// defines the supported db version
#define DB_VERSION_PARAMETER "database_version"
#define SUPPORTED_DB_VERSION "7"


/**
 * Constructor.
 */
DALStorage::DALStorage()
        : mDb(dal::DataProviderFactory::createDataProvider()),
          mItemDbVersion(0)
{
}

/**
 * Destructor.
 */
DALStorage::~DALStorage()
{
    if (mDb->isConnected()) {
        close();
    }
    delete mDb;
}

/**
 * Connect to the database and initialize it if necessary.
 *
 */
void DALStorage::open()
{
    // Do nothing if already connected.
    if (mDb->isConnected())
    {
        return;
    }

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
        SyncDatabase();

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
void DALStorage::close()
{
    mDb->disconnect();
}

Account *DALStorage::getAccountBySQL()
{
    try {
//        const dal::RecordSet &accountInfo = mDb->execSql(query);
        const dal::RecordSet &accountInfo = mDb->processSql();

        // if the account is not even in the database then
        // we have no choice but to return nothing.
        if (accountInfo.isEmpty())
        {
            return NULL;
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
                || time(NULL) <= (int) toUint(accountInfo(0, 5)))
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
        return NULL; // TODO: Throw exception here
    }
}

/**
 * Get an account by user name.
 */
Account *DALStorage::getAccount(const std::string &userName)
{
    std::ostringstream sql;
//    sql << "select * from " << ACCOUNTS_TBL_NAME << " where username = \"" << userName << "\";";
    sql << "SELECT * FROM " << ACCOUNTS_TBL_NAME << " WHERE username = ?";
    if (mDb->prepareSql(sql.str()))
    {
        mDb->bindValue(1, userName);
    }
    return getAccountBySQL();
}

/**
 * Get an account by ID.
 */
Account *DALStorage::getAccount(int accountID)
{
    std::ostringstream sql;
//    sql << "select * from " << ACCOUNTS_TBL_NAME << " where id = '" << accountID << "';";
    sql << "SELECT * FROM " << ACCOUNTS_TBL_NAME << " WHERE id = ?";
    if (mDb->prepareSql(sql.str()))
    {
        mDb->bindValue(1, accountID);
    }
    return getAccountBySQL();
}

Character *DALStorage::getCharacterBySQL(Account *owner)
{
    Character *character;

    // specialize the string_to functor to convert
    // a string to an unsigned int.
    string_to< unsigned > toUint;

    try {
//        const dal::RecordSet &charInfo = mDb->execSql(query);
        const dal::RecordSet &charInfo = mDb->processSql();

        // if the character is not even in the database then
        // we have no choice but to return nothing.
        if (charInfo.isEmpty())
        {
            return NULL;
        }

        // specialize the string_to functor to convert
        // a string to an unsigned short.
        string_to< unsigned short > toUshort;

        character = new Character(charInfo(0, 2), toUint(charInfo(0, 0)));
        character->setGender(toUshort(charInfo(0, 3)));
        character->setHairStyle(toUshort(charInfo(0, 4)));
        character->setHairColor(toUshort(charInfo(0, 5)));
        character->setLevel(toUshort(charInfo(0, 6)));
        character->setCharacterPoints(toUshort(charInfo(0, 7)));
        character->setCorrectionPoints(toUshort(charInfo(0, 8)));
        character->getPossessions().money = toUint(charInfo(0, 9));
        Point pos(toUshort(charInfo(0, 10)), toUshort(charInfo(0, 11)));
        character->setPosition(pos);
        for (int i = 0; i < CHAR_ATTR_NB; ++i)
        {
            character->setAttribute(CHAR_ATTR_BEGIN + i,
                                    toUshort(charInfo(0, 13 + i)));
        }

        int mapId = toUint(charInfo(0, 12));
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

        // load the skills of the char from CHAR_SKILLS_TBL_NAME
        std::ostringstream s;
        s << "SELECT skill_id, skill_exp "
          << "FROM " << CHAR_SKILLS_TBL_NAME << " "
          << "WHERE char_id = " << character->getDatabaseID();

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
        s.clear();
        s.str("");
        // Load the status effect
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
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::getCharacter #1) SQL query failure: " << e.what());
        return NULL;
    }

    try
    {
        std::ostringstream sql;
        sql << " select * from " << INVENTORIES_TBL_NAME << " where owner_id = '"
            << character->getDatabaseID() << "' order by slot asc;";

        const dal::RecordSet &itemInfo = mDb->execSql(sql.str());
        if (!itemInfo.isEmpty())
        {
            Possessions &poss = character->getPossessions();
            unsigned nextSlot = 0;

            for (int k = 0, size = itemInfo.rows(); k < size; ++k)
            {
                unsigned slot = toUint(itemInfo(k, 2));
                if (slot < EQUIPMENT_SLOTS)
                {
                    poss.equipment[slot] = toUint(itemInfo(k, 3));
                }
                else
                {
                    slot -= 32;
                    if (slot >= INVENTORY_SLOTS || slot < nextSlot)
                    {
                        LOG_ERROR("(DALStorage::getCharacter #2) Corrupted inventory.");
                        break;
                    }
                    InventoryItem item;
                    if (slot != nextSlot)
                    {
                        item.itemId = 0;
                        item.amount = slot - nextSlot;
                        poss.inventory.push_back(item);
                    }
                    item.itemId = toUint(itemInfo(k, 3));
                    item.amount = toUint(itemInfo(k, 4));
                    poss.inventory.push_back(item);
                    nextSlot = slot + 1;
                }
            }
        }
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::getCharacter #2) SQL query failure: " << e.what());
        return NULL;
    }

    return character;
}

/**
 * Gets a character by database ID.
 */
Character *DALStorage::getCharacter(int id, Account *owner)
{
    std::ostringstream sql;
//    sql << "select * from " << CHARACTERS_TBL_NAME << " where id = '" << id << "';";
    sql << "SELECT * FROM " << CHARACTERS_TBL_NAME << " WHERE id = ?";
    if (mDb->prepareSql(sql.str()))
    {
        mDb->bindValue(1, id);
    }
    return getCharacterBySQL(owner);
}

Character *DALStorage::getCharacter(const std::string &name)
{
    std::ostringstream sql;
    sql << "SELECT * FROM " << CHARACTERS_TBL_NAME << " WHERE name = ?";
    if (mDb->prepareSql(sql.str()))
    {
        mDb->bindValue(1, name);
    }
    return getCharacterBySQL(NULL);
}

#if 0
/**
 * Return the list of all Emails addresses.
 */
std::list<std::string>
DALStorage::getEmailList()
{
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
#endif

bool DALStorage::doesUserNameExist(const std::string &name)
{
    try {
        std::ostringstream sql;
//        sql << "select count(username) from " << ACCOUNTS_TBL_NAME
//            << " where username = \"" << name << "\";";
//        const dal::RecordSet &accountInfo = mDb->execSql(sql.str());
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
 * Tells if the email address already exists
 * @return true if the email address exists.
 */
bool DALStorage::doesEmailAddressExist(const std::string &email)
{
    try {
        std::ostringstream sql;
//        sql << "select count(email) from " << ACCOUNTS_TBL_NAME
//            << " where upper(email) = upper(\"" << email << "\");";
//        const dal::RecordSet &accountInfo = mDb->execSql(sql.str());
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
 * Tells if the character's name already exists
 * @return true if character's name exists.
 */
bool DALStorage::doesCharacterNameExist(const std::string& name)
{
    try {
        std::ostringstream sql;
//        sql << "select count(name) from " << CHARACTERS_TBL_NAME
//            << " where name = \"" << name << "\";";
//        const dal::RecordSet &accountInfo = mDb->execSql(sql.str());
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

bool DALStorage::updateCharacter(Character *character,
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
            << "money = '"      << character->getPossessions().money << "', "
            << "x = '"          << character->getPosition().x << "', "
            << "y = '"          << character->getPosition().y << "', "
            << "map_id = '"     << character->getMapId() << "', "
            << "str = '"        << character->getAttribute(CHAR_ATTR_STRENGTH) << "', "
            << "agi = '"        << character->getAttribute(CHAR_ATTR_AGILITY) << "', "
            << "dex = '"        << character->getAttribute(CHAR_ATTR_DEXTERITY) << "', "
            << "vit = '"        << character->getAttribute(CHAR_ATTR_VITALITY) << "', "
#if defined(MYSQL_SUPPORT) || defined(POSTGRESQL_SUPPORT)
            << "`int` = '"
#else
            << "int = '"
#endif
                                << character->getAttribute(CHAR_ATTR_INTELLIGENCE) << "', "
            << "will = '"       << character->getAttribute(CHAR_ATTR_WILLPOWER) << "' "
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
     *  Character's skills
     */
    try
    {
        std::map<int, int>::const_iterator skill_it;
        for (skill_it = character->getSkillBegin();
             skill_it != character->getSkillEnd(); skill_it++)
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
        LOG_ERROR("(DALStorage::updateCharacter #2) SQL query failure: " << e.what());
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
        LOG_ERROR("(DALStorage::updateCharacter #3) SQL query failure: " << e.what());
        return false;
    }

    // Insert the new inventory data
    try
    {
        std::ostringstream sql;

        sql << "insert into " << INVENTORIES_TBL_NAME
            << " (owner_id, slot, class_id, amount) values ("
            << character->getDatabaseID() << ", ";
        std::string base = sql.str();

        const Possessions &poss = character->getPossessions();

        for (int j = 0; j < EQUIPMENT_SLOTS; ++j)
        {
            int v = poss.equipment[j];
            if (!v) continue;
            sql.str(std::string());
            sql << base << j << ", " << v << ", 1);";
            mDb->execSql(sql.str());
        }

        int slot = 32;
        for (std::vector< InventoryItem >::const_iterator j = poss.inventory.begin(),
             j_end = poss.inventory.end(); j != j_end; ++j)
        {
            int v = j->itemId;
            if (!v)
            {
                slot += j->amount;
                continue;
            }
            sql.str(std::string());
            sql << base << slot << ", " << v << ", " << unsigned(j->amount) << ");";
            mDb->execSql(sql.str());
            ++slot;
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
        LOG_ERROR("(DALStorage::updateCharacter #5) SQL query failure: " << e.what());
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
        LOG_ERROR("(DALStorage::updateCharacter #6) SQL query failure: " << e.what());
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
 * @deprecated Use DALStorage::updateExperience instead!!!
*/
void DALStorage::flushSkill(const Character* const character,
                            const int skill_id )
{
    updateExperience(character->getDatabaseID(), skill_id,
        character->getExperience(skill_id));
}

/**
 * Add an account to the database.
 */
void DALStorage::addAccount(Account *account)
{
    assert(account->getCharacters().size() == 0);

    using namespace dal;

    mDb->beginTransaction();
    try
    {
        // insert the account.
        std::ostringstream sql;
        sql << "insert into " << ACCOUNTS_TBL_NAME
             << " (username, password, email, level, banned, registration, lastlogin)"
//             << " values (\""
//             << account->getName() << "\", \""
//             << account->getPassword() << "\", \""
//             << account->getEmail() << "\", "
//             << account->getLevel() << ", 0, "
//             << account->getRegistrationDate() << ", "
//             << account->getLastLogin() << ");";
//        mDb->execSql(sql1.str());
            << " VALUES (?, ?, ?, " << account->getLevel() << ", 0, "
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
void DALStorage::flush(Account *account)
{
    assert(account->getID() >= 0);

    using namespace dal;

    mDb->beginTransaction();
    try
    {

        // update the account.
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
                     << " (user_id, name, gender, hair_style, hair_color, level, char_pts, correct_pts, money,"
                     << " x, y, map_id, str, agi, dex, vit, "
#if defined(MYSQL_SUPPORT) || defined(POSTGRESQL_SUPPORT)
            << "`int`, "
#else
            << "int, "
#endif
                     << "will ) values ("
                     << account->getID() << ", \""
                     << (*it)->getName() << "\", "
                     << (*it)->getGender() << ", "
                     << (int)(*it)->getHairStyle() << ", "
                     << (int)(*it)->getHairColor() << ", "
                     << (int)(*it)->getLevel() << ", "
                     << (int)(*it)->getCharacterPoints() << ", "
                     << (int)(*it)->getCorrectionPoints() << ", "
                     << (*it)->getPossessions().money << ", "
                     << (*it)->getPosition().x << ", "
                     << (*it)->getPosition().y << ", "
                     << (*it)->getMapId() << ", "
                     << (*it)->getAttribute(CHAR_ATTR_STRENGTH) << ", "
                     << (*it)->getAttribute(CHAR_ATTR_AGILITY) << ", "
                     << (*it)->getAttribute(CHAR_ATTR_DEXTERITY) << ", "
                     << (*it)->getAttribute(CHAR_ATTR_VITALITY) << ", "
                     << (*it)->getAttribute(CHAR_ATTR_INTELLIGENCE) << ", "
                     << (*it)->getAttribute(CHAR_ATTR_WILLPOWER) << " "
                     << ");";

                mDb->execSql(sqlInsertCharactersTable.str());

                // Update the character ID.
                (*it)->setDatabaseID(mDb->getLastId());

                // update the characters skill
                std::map<int, int>::const_iterator skill_it;
                for (skill_it = (*it)->getSkillBegin();
                     skill_it != (*it)->getSkillEnd(); skill_it++)
                {
                    updateExperience((*it)->getDatabaseID(), skill_it->first, skill_it->second);
                }
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
 */
void DALStorage::delAccount(Account *account)
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
 */
void DALStorage::updateLastLogin(const Account *account)
{
    std::ostringstream sql;
    sql << "UPDATE " << ACCOUNTS_TBL_NAME
        << "   SET lastlogin = '" << account->getLastLogin() << "'"
        << " WHERE id = '" << account->getID() << "';";
    mDb->execSql(sql.str());
}

void DALStorage::updateCharacterPoints(const int CharId, const int CharPoints,
    const int CorrPoints, const int AttribId, const int AttribValue )
{
    std::ostringstream sql;
    sql << "UPDATE " << CHARACTERS_TBL_NAME
        << " SET char_pts = " << CharPoints << ", "
        << " correct_pts = " << CorrPoints << ", ";

    switch (AttribId)
    {
        case CHAR_ATTR_STRENGTH:     sql << "str = "; break;
        case CHAR_ATTR_AGILITY:      sql << "agi = "; break;
        case CHAR_ATTR_DEXTERITY:    sql << "dex = "; break;
        case CHAR_ATTR_VITALITY:     sql << "vit = "; break;
        case CHAR_ATTR_INTELLIGENCE: sql << "int = "; break;
        case CHAR_ATTR_WILLPOWER:    sql << "will = "; break;
    }
    sql << AttribValue
        << " WHERE id = " << CharId;

    mDb->execSql(sql.str());
}

void DALStorage::updateExperience(const int CharId, const int SkillId,
    const int SkillValue)
{
    try
    {
        // if experience has decreased to 0 we don't store it anymore,
        // its the default
        if (SkillValue == 0)
        {
            std::ostringstream sql;
            sql << "DELETE FROM " << CHAR_SKILLS_TBL_NAME
                << " WHERE char_id = " << CharId
                << " AND skill_id = " << SkillId;
            mDb->execSql(sql.str());
            return;
        }

        // try to update the skill
        std::ostringstream sql;
        sql << "UPDATE " << CHAR_SKILLS_TBL_NAME
            << " SET skill_exp = " << SkillValue
            << " WHERE char_id = " << CharId
            << " AND skill_id = " << SkillId;
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
            << CharId << ", "
            << SkillId << ", "
            << SkillValue << ")";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("DALStorage::updateExperience: " << e.what());
        throw;
    }
}

void DALStorage::insertStatusEffect(const int charId, const int statusId, const int time)
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
 * Add a guild
 */
void DALStorage::addGuild(Guild* guild)
{
    std::ostringstream insertSql;
    insertSql << "insert into " << GUILDS_TBL_NAME
        << " (name) VALUES (?)";
    if (mDb->prepareSql(insertSql.str()))
    {
        mDb->bindValue(1, guild->getName());
    }
    //mDb->execSql(insertSql.str());
    mDb->processSql();

    std::ostringstream selectSql;
    selectSql << "SELECT id FROM " << GUILDS_TBL_NAME
        << " WHERE name = ?";

    if (mDb->prepareSql(selectSql.str()))
    {
        mDb->bindValue(1, guild->getName());
    }
    //const dal::RecordSet& guildInfo = mDb->execSql(selectSql.str());
    const dal::RecordSet& guildInfo = mDb->processSql();

    string_to<unsigned int> toUint;
    unsigned id = toUint(guildInfo(0, 0));
    guild->setId(id);
}

/**
 * Remove guild
 */
void DALStorage::removeGuild(Guild* guild)
{
    std::ostringstream sql;
    sql << "delete from " << GUILDS_TBL_NAME
        << " where id = '"
        << guild->getId() << "';";
    mDb->execSql(sql.str());
}

/**
 * add a member to a guild
 */
void DALStorage::addGuildMember(int guildId, int memberId)
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
* remove a member from a guild
 */
void DALStorage::removeGuildMember(int guildId, int memberId)
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

void DALStorage::setMemberRights(int guildId, int memberId, int rights)
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
 * get a list of guilds
 */
std::list<Guild*> DALStorage::getGuildList()
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
                Character *character = getCharacter((*i).first, NULL);
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

std::string DALStorage::getQuestVar(int id, const std::string &name)
{
    try
    {
        std::ostringstream query;
        query << "select value from " << QUESTS_TBL_NAME
//              << " where owner_id = '" << id << "' and name = '"
//              << name << "';";
//        const dal::RecordSet &info = mDb->execSql(query.str());
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

std::string DALStorage::getWorldStateVar(const std::string &name, int map_id)
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

void DALStorage::setWorldStateVar(const std::string &name, const std::string &value)
{
    return setWorldStateVar(name, -1, value);
}

void DALStorage::setWorldStateVar(const std::string &name,
                                  int map_id,
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
            if (map_id >= 0)
            {
                deleteStateVar << " AND map_id = '" << map_id << "'";
            }
            deleteStateVar << ";";
            mDb->execSql(deleteStateVar.str());
            return;
        }

        // try to update the variable in the database
        std::ostringstream updateStateVar;
        updateStateVar << "UPDATE " << WORLD_STATES_TBL_NAME
                       << "   SET value = '" << value << "', "
                       << "       moddate = '" << time(NULL) << "' "
                       << " WHERE state_name = '" << name << "'";

        if (map_id >= 0)
        {
            updateStateVar << "   AND map_id = '" << map_id << "'";
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
        if (map_id >= 0)
        {
            insertStateVar << "'" << map_id << "', ";
        }
        else
        {
            insertStateVar << "NULL , ";
        }
        insertStateVar << "'" << value << "', "
                       << "'" << time(NULL) << "');";
        mDb->execSql(insertStateVar.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::setWorldStateVar) SQL query failure: " << e.what());
    }
}

void DALStorage::setQuestVar(int id, const std::string &name,
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

void DALStorage::banCharacter(int id, int duration)
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
            << time(NULL) + duration * 60
            << "' where id = '" << info(0, 0) << "';";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::banAccount) SQL query failure: " << e.what());
    }
}

void DALStorage::delCharacter(int charId, bool startTransaction = true) const
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

void DALStorage::delCharacter(Character *character,
                              bool startTransaction = true) const
{
    delCharacter(character->getDatabaseID(), startTransaction);
}

void DALStorage::checkBannedAccounts()
{
    try
    {
        // update expired bans
        std::ostringstream sql;
        sql << "update " << ACCOUNTS_TBL_NAME
        << " set level = " << AL_PLAYER << ", banned = 0"
        << " where level = " << AL_BANNED
        << " AND banned <= " << time(NULL) << ";";
        mDb->execSql(sql.str());
    }
    catch (const dal::DbSqlQueryExecFailure &e)
    {
        LOG_ERROR("(DALStorage::checkBannedAccounts) SQL query failure: " << e.what());
    }
}

void DALStorage::setAccountLevel(int id, int level)
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

void DALStorage::setPlayerLevel(int id, int level)
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

void DALStorage::storeLetter(Letter *letter)
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
            << time(NULL) << ", "
//            << "'" << letter->getContents() << "' )";
            << "?)";
        if (mDb->prepareSql(sql.str()))
        {
            mDb->bindValue(1, letter->getContents());
        }

        mDb->processSql();
//        mDb->execSql(sql.str());
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
            << "       sending_date    = '" << time(NULL) << "', "
//            << "       letter_text     = '" << letter->getContents() << "' "
            << "       letter_text = ? "
            << " WHERE letter_id       = '" << letter->getId() << "'";

        if (mDb->prepareSql(sql.str()))
        {
            mDb->bindValue(1, letter->getContents());
        }
        mDb->processSql();
        //mDb->execSql(sql.str());

        if (mDb->getModifiedRows() == 0)
        {
            // this should never happen...
            LOG_ERROR("(DALStorage::storePost) trying to update nonexistant letter");
            throw "(DALStorage::storePost) trying to update nonexistant letter";
        }

        // TODO: update attachments in the database
    }
}

Post* DALStorage::getStoredPost(int playerId)
{
    Post* p = new Post();
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
        Character *sender = getCharacter(toUint(post(i, 1)), NULL);
        Character *receiver = getCharacter(toUint(post(i, 2)), NULL);

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

void DALStorage::deletePost(Letter* letter)
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
    catch(const dal::DbSqlQueryExecFailure &e)
    {
        mDb->rollbackTransaction();
        LOG_ERROR("(DALStorage::deletePost) SQL query failure: " << e.what());
    }
}

void DALStorage::SyncDatabase(void)
{
    xmlDocPtr doc = xmlReadFile(DEFAULT_ITEM_FILE, NULL, 0);
    if (!doc)
    {
        LOG_ERROR("Item Manager: Error while parsing item database (items.xml)!");
        return;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node || !xmlStrEqual(node->name, BAD_CAST "items"))
    {
        LOG_ERROR("Item Manager:(items.xml) is not a valid database file!");
        xmlFreeDoc(doc);
        return;
    }

    mDb->beginTransaction();
    int itmCount = 0;
    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
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
        {
            continue;
        }

        if (xmlStrEqual(node->name, BAD_CAST "item"))
        {
            int id = XML::getProperty(node, "id", 0);
            if (id < 500)
            {
                continue;
            }

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
//                    << " SET name = '" << mDb->escapeSQL(name) << "', "
                    << " SET name = ?, "
//                    << "     description = '" << mDb->escapeSQL(desc) << "', "
                    << "     description = ?, "
                    << "     image = '" << image << "', "
                    << "     weight = " << weight << ", "
                    << "     itemtype = '" << type << "', "
//                    << "     effect = '" << mDb->escapeSQL(eff) << "', "
                    << "     effect = ?, "
                    << "     dyestring = '" << dye << "' "
                    << " WHERE id = " << id;

//                mDb->execSql(sql.str());
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
                    //mDb->execSql(sql.str());
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
    xmlFreeDoc(doc);
}

void DALStorage::setOnlineStatus(int charId, bool online)
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
                << " VALUES (" << charId << ", " << time(NULL) << ")";
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

void DALStorage::addTransaction(const Transaction &trans)
{
    try
    {
        std::stringstream sql;
        sql << "INSERT INTO " << TRANSACTION_TBL_NAME
            << " VALUES (NULL, " << trans.mCharacterId << ", "
            << trans.mAction << ", "
            << "?, "
            << time(NULL) << ")";
//            << ", '" << trans.mMessage << "', " << time(NULL) << ")";
//        mDb->execSql(sql.str());
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

std::vector<Transaction> DALStorage::getTransactions(unsigned int num)
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

std::vector<Transaction> DALStorage::getTransactions(time_t date)
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
