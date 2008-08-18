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
#include <time.h>

#include "account-server/dalstorage.hpp"

#include "point.h"
#include "account-server/account.hpp"
#include "account-server/dalstoragesql.hpp"
#include "chat-server/chatchannel.hpp"
#include "chat-server/guild.hpp"
#include "common/configuration.hpp"
#include "dal/dalexcept.h"
#include "dal/dataproviderfactory.h"
#include "utils/functors.h"
#include "utils/logger.h"

/**
 * Constructor.
 */
DALStorage::DALStorage()
        : mDb(dal::DataProviderFactory::createDataProvider())
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
 */
void DALStorage::open()
{
    // Do nothing if already connected.
    if (mDb->isConnected()) {
        return;
    }

    using namespace dal;

    static bool dbFileShown = false;
    std::string dbFile = "tmw";
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
        createTable(INVENTORIES_TBL_NAME, SQL_INVENTORIES_TABLE);
        createTable(GUILDS_TBL_NAME, SQL_GUILDS_TABLE);
        createTable(GUILD_MEMBERS_TBL_NAME, SQL_GUILD_MEMBERS_TABLE);
        createTable(QUESTS_TBL_NAME, SQL_QUESTS_TABLE);
    }
    catch (const DbConnectionFailure& e) {
        LOG_ERROR("(DALStorage::open #1) Unable to connect to the database: "
                     << e.what());
    }
    catch (const DbSqlQueryExecFailure& e) {
        LOG_ERROR("(DALStorage::open #2) SQL query failure: " << e.what());
    }
}


/**
 * Disconnect from the database.
 */
void DALStorage::close()
{
    mDb->disconnect();
}


Account *DALStorage::getAccountBySQL(std::string const &query)
{
    try {
        dal::RecordSet const &accountInfo = mDb->execSql(query);

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
        dal::RecordSet const &charInfo = mDb->execSql(sql.str());

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
    catch (dal::DbSqlQueryExecFailure const &e)
    {
        return NULL; // TODO: Throw exception here
    }
}


/**
 * Get an account by user name.
 */
Account *DALStorage::getAccount(std::string const &userName)
{
    std::ostringstream sql;
    sql << "select * from " << ACCOUNTS_TBL_NAME << " where username = \"" << userName << "\";";
    return getAccountBySQL(sql.str());
}


/**
 * Get an account by ID.
 */
Account *DALStorage::getAccount(int accountID)
{
    std::ostringstream sql;
    sql << "select * from " << ACCOUNTS_TBL_NAME << " where id = '" << accountID << "';";
    return getAccountBySQL(sql.str());
}


Character *DALStorage::getCharacterBySQL(std::string const &query, Account *owner)
{
    Character *character;

    // specialize the string_to functor to convert
    // a string to an unsigned int.
    string_to< unsigned > toUint;

    try {
        dal::RecordSet const &charInfo = mDb->execSql(query);

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
        for (int i = 0; i < CHAR_SKILL_WEAPON_NB; ++i)
        {
            int exp = toUint(charInfo(0, 13 + CHAR_ATTR_NB + i));
            character->setExperience(i, exp);
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
            dal::RecordSet const &levelInfo = mDb->execSql(s.str());
            character->setAccountLevel(toUint(levelInfo(0, 0)), true);
        }

    }
    catch (dal::DbSqlQueryExecFailure const &e)
    {
        LOG_ERROR("(DALStorage::getCharacter #1) SQL query failure: " << e.what());
        return NULL;
    }

    try
    {
        std::ostringstream sql;
        sql << " select * from " << INVENTORIES_TBL_NAME << " where owner_id = '"
            << character->getDatabaseID() << "' order by slot asc;";

        dal::RecordSet const &itemInfo = mDb->execSql(sql.str());
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
    catch (dal::DbSqlQueryExecFailure const &e)
    {
        LOG_ERROR("(DALStorage::getCharacter #2) SQL query failure: " << e.what());
    }

    return character;
}


/**
 * Gets a character by database ID.
 */
Character *DALStorage::getCharacter(int id, Account *owner)
{
    std::ostringstream sql;
    sql << "select * from " << CHARACTERS_TBL_NAME << " where id = '" << id << "';";
    return getCharacterBySQL(sql.str(), owner);
}

Character *DALStorage::getCharacter(const std::string &name)
{
    std::ostringstream sql;
    sql << "select * from " << CHARACTERS_TBL_NAME << " where name = '" << name << "';";
    return getCharacterBySQL(sql.str(), NULL);
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

bool DALStorage::doesUserNameExist(std::string const &name)
{
    try {
        std::ostringstream sql;
        sql << "select count(username) from " << ACCOUNTS_TBL_NAME
            << " where username = \"" << name << "\";";
        dal::RecordSet const &accountInfo = mDb->execSql(sql.str());

        std::istringstream ssStream(accountInfo(0, 0));
        unsigned int iReturn = 1;
        ssStream >> iReturn;
        return iReturn != 0;
    } catch (std::exception const &e) {
        // TODO: throw an exception.
        LOG_ERROR("(DALStorage::doesUserNameExist) SQL query failure: " << e.what());
    }

    return true;
}

/**
 * Tells if the email address already exists
 * @return true if the email address exists.
 */
bool DALStorage::doesEmailAddressExist(std::string const &email)
{
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

bool DALStorage::updateCharacter(Character *character)
{
    // Update the database Character data (see CharacterData for details)
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
            << "will = '"       << character->getAttribute(CHAR_ATTR_WILLPOWER) << "', "
            << "unarmed_exp = '"<< character->getExperience(CHAR_SKILL_WEAPON_NONE - CHAR_SKILL_BEGIN) << "', "
            << "knife_exp = '"  << character->getExperience(CHAR_SKILL_WEAPON_KNIFE - CHAR_SKILL_BEGIN) << "', "
            << "sword_exp = '"  << character->getExperience(CHAR_SKILL_WEAPON_SWORD - CHAR_SKILL_BEGIN) << "', "
            << "polearm_exp = '"<< character->getExperience(CHAR_SKILL_WEAPON_POLEARM - CHAR_SKILL_BEGIN) << "', "
            << "staff_exp = '"  << character->getExperience(CHAR_SKILL_WEAPON_STAFF - CHAR_SKILL_BEGIN) << "', "
            << "whip_exp = '"   << character->getExperience(CHAR_SKILL_WEAPON_WHIP - CHAR_SKILL_BEGIN) << "', "
            << "bow_exp = '"    << character->getExperience(CHAR_SKILL_WEAPON_BOW - CHAR_SKILL_BEGIN) << "', "
            << "shoot_exp = '"  << character->getExperience(CHAR_SKILL_WEAPON_SHOOTING - CHAR_SKILL_BEGIN) << "', "
            << "mace_exp = '"   << character->getExperience(CHAR_SKILL_WEAPON_MACE - CHAR_SKILL_BEGIN) << "', "
            << "axe_exp = '"    << character->getExperience(CHAR_SKILL_WEAPON_AXE - CHAR_SKILL_BEGIN) << "', "
            << "thrown_exp = '" << character->getExperience(CHAR_SKILL_WEAPON_THROWN - CHAR_SKILL_BEGIN) << "' "

            << "where id = '"   << character->getDatabaseID() << "';";

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
    try
    {
        std::ostringstream sql;

        sql << "insert into " << INVENTORIES_TBL_NAME
            << " (owner_id, slot, class_id, amount) values ("
            << character->getDatabaseID() << ", ";
        std::string base = sql.str();

        Possessions const &poss = character->getPossessions();

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
        LOG_ERROR("(DALStorage::updateCharacter #3) SQL query failure: " << e.what());
        return false;
    }

    return true;
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
void DALStorage::addAccount(Account *account)
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
}

/**
 * Update an account from the database.
 */
void DALStorage::flush(Account *account)
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
        if ((*it)->getDatabaseID() >= 0)
        {
            updateCharacter(*it);
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
                 << " x, y, map_id, str, agi, dex, vit, int, will, unarmed_exp, knife_exp, sword_exp, polearm_exp,"
                 << " staff_exp, whip_exp, bow_exp, shoot_exp, mace_exp, axe_exp, thrown_exp) values ("
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
                 << (*it)->getAttribute(CHAR_ATTR_WILLPOWER) << ", "
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_NONE - CHAR_SKILL_BEGIN) << ", "
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_KNIFE - CHAR_SKILL_BEGIN) << ","
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_SWORD - CHAR_SKILL_BEGIN) << ", "
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_POLEARM - CHAR_SKILL_BEGIN) << ", "
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_STAFF - CHAR_SKILL_BEGIN) << ","
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_WHIP - CHAR_SKILL_BEGIN) << ", "
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_BOW - CHAR_SKILL_BEGIN) << ", "
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_SHOOTING - CHAR_SKILL_BEGIN) << ", "
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_MACE - CHAR_SKILL_BEGIN) << ", "
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_AXE - CHAR_SKILL_BEGIN) << ", "
                 << (*it)->getExperience(CHAR_SKILL_WEAPON_THROWN - CHAR_SKILL_BEGIN)
                 << ");";

            mDb->execSql(sqlInsertCharactersTable.str());

            // Update the character ID.
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
 * Add a guild
 */
void DALStorage::addGuild(Guild* guild)
{
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

void DALStorage::setMemberRights(int memberId, int rights)
{
    std::ostringstream sql;

    try
    {
        sql << "update " << GUILD_MEMBERS_TBL_NAME
        << " set rights = '" << rights << "'"
        << " where member_id = \""
        << memberId << "\";";
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

std::string DALStorage::getQuestVar(int id, std::string const &name)
{
    try
    {
        std::ostringstream query;
        query << "select value from " << QUESTS_TBL_NAME
              << " where owner_id = '" << id << "' and name = '"
              << name << "';";
        dal::RecordSet const &info = mDb->execSql(query.str());

        if (!info.isEmpty()) return info(0, 0);
    }
    catch (dal::DbSqlQueryExecFailure const &e)
    {
        LOG_ERROR("(DALStorage::getQuestVar) SQL query failure: " << e.what());
    }

    return std::string();
}

void DALStorage::setQuestVar(int id, std::string const &name,
                             std::string const &value)
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
    catch (dal::DbSqlQueryExecFailure const &e)
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
        dal::RecordSet const &info = mDb->execSql(query.str());
        if (info.isEmpty())
        {
            LOG_ERROR("Tried to ban an unknown user.");
            return;
        }

        std::ostringstream sql;
        sql << "update " << ACCOUNTS_TBL_NAME
            << " set banned = '" << time(NULL) + duration * 60
            << "' where id = '" << info(0, 0) << "';";
        mDb->execSql(sql.str());
    }
    catch (dal::DbSqlQueryExecFailure const &e)
    {
        LOG_ERROR("(DALStorage::banAccount) SQL query failure: " << e.what());
    }
}
