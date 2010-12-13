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

#ifndef STORAGE_H
#define STORAGE_H

#include <list>
#include <map>
#include <vector>

#include "dal/dataprovider.h"

#include "common/transaction.h"

class Account;
class Character;
class ChatChannel;
class Guild;
class Letter;
class Post;

/**
 * The high level interface to the database. Through the storage you can access
 * all accounts, characters, guilds, worlds states, transactions, etc.
 */
class Storage
{
    public:
        Storage();
        ~Storage();

        /**
         * Connect to the database and initialize it if necessary.
         */
        void open();

        /**
         * Disconnect from the database.
         */
        void close();

        /**
         * Get an account by user name.
         *
         * @param userName the owner of the account.
         *
         * @return the account associated to the user name.
         */
        Account *getAccount(const std::string &userName);

        /**
         * Get an account by Id.
         *
         * @param accountID the Id of the account.
         *
         * @return the account associated with the Id.
         */
        Account *getAccount(int accountId);

        /**
         * Gets a character by database Id.
         *
         * @param id the ID of the character.
         * @param owner the account the character is in.
         *
         * @return the character associated to the Id.
         */
        Character *getCharacter(int id, Account *owner);

        /**
         * Gets a character by character name.
         *
         * @param name of the character
         *
         * @return the character associated to the name
         */
        Character *getCharacter(const std::string &name);

        /**
         * Add an account to the database.
         *
         * @param account the new account.
         */
        void addAccount(Account *account);

        /**
         * Delete an account and its associated data from the database.
         *
         * @param account the account to delete.
         */
        void delAccount(Account *account);

        /**
         * Update the date and time of the last login.
         *
         * @param account the account that recently logged in.
         */
        void updateLastLogin(const Account *account);

        /**
         * Write a modification message about Character points to the database.
         *
         * @param CharId      ID of the character
         * @param CharPoints  Number of character points left for the character
         * @param CorrPoints  Number of correction points left for the character
         */
        void updateCharacterPoints(int charId,
                                   int charPoints, int corrPoints);

        /**
         * Write a modification message about character skills to the database.
         *
         * @param CharId      ID of the character
         * @param SkillId     ID of the skill
         * @param SkillValue  new skill points
         */
        void updateExperience(int charId, int skillId, int skillValue);

        /**
         * Write a modification message about character attributes
         * to the database.
         *
         * @param charId    The Id of the character
         * @param attrId    The Id of the attribute
         * @param base      The base value of the attribute for this character
         * @param mod       The cached modified value for this character.
         */
        void updateAttribute(int charId, unsigned int attrId,
                             double base, double mod);

        /**
         * Write a modification message about character skills to the database.
         *
         * @param CharId      ID of the character
         * @param monsterId   ID of the monster type
         * @param kills       new amount of kills
         */
        void updateKillCount(int charId, int monsterId, int kills);

        /**
         * Inserts a record about a status effect into the database
         *
         * @param charId    ID of the character in the database
         * @param statusId  ID of the status effect
         * @param time      Time left on the status effect
         */
        void insertStatusEffect(int charId, int statusId, int time);

        /**
         * Sets a ban on an account (hence on all its characters).
         *
         * @param id character identifier.
         * @param duration duration in minutes.
         */
        void banCharacter(int id, int duration);

        /**
         * Delete a character in the database.
         *
         * @param charId character identifier.
         */
        void delCharacter(int charId) const;

        /**
         * Delete a character in the database. The object itself is not touched
         * by this function!
         *
         * @param character character object.
         */
        void delCharacter(Character *character) const;

        /**
         * Removes expired bans from accounts
         */
        void checkBannedAccounts();

        /**
         * Tells if the user name already exists.
         *
         * @param name The user name to check
         *
         * @return true if the user name exists.
         */
        bool doesUserNameExist(const std::string &name);

        /**
         * Tells if the email address already exists.
         *
         * @param email The email address to check
         *
         * @return true if the email address exists.
         */
        bool doesEmailAddressExist(const std::string &email);

        /**
         * Tells if the character's name already exists.
         *
         * @param name The character name to check
         *
         * @return true if character's name exists.
         */
        bool doesCharacterNameExist(const std::string &name);

        /**
         * Updates the data for a single character,
         * does not update the owning account or the characters name.
         * Primary usage should be storing characterdata
         * received from a game server.
         *
         * @param ptr Character to store values in the database.
         *
         * @return true on success
         */
        bool updateCharacter(Character *ptr);

        /**
         * Save changes of a skill to the database permanently.
         *
         * @param character Character thats skill has changed.
         * @param skill_id Identifier of the changed skill.
         *
         * @exception dbl::DbSqlQueryExecFailure.
         * @deprecated Use DALStorage::updateExperience instead!!!
         */
        void flushSkill(const Character *character, int skillId);

        /**
         * Add a new guild.
         *
         * @param guild The guild to add in database.
         */
        void addGuild(Guild *guild);

        /**
         * Delete a guild.
         *
         * @param guild The guild to remove from database.
         */
        void removeGuild(Guild *guild);

        /**
         * Add member to guild.
         *
         * @param guildId The guild Id where to add the member
         * @param memberId The character Id to add in the guild.
         */
        void addGuildMember(int guildId, int memberId);

        /**
         * Remove member from guild.
         *
         * @param guildId The guild Id where to remove the member
         * @param memberId The character Id to remove from the guild.
         */
        void removeGuildMember(int guildId, int memberId);

        /**
         * Save guild member rights.
         *
         * @param guildId The guild Id where to set the member's right
         * @param memberId The character Id
         * @param memberId The new right level of the member.
         */
        void setMemberRights(int guildId, int memberId, int rights);

        /**
         * Get the list of guilds.
         *
         * @return a list of guilds
         */
        std::list<Guild*> getGuildList();

        /**
         * Update an account to the database.
         *
         * @param Account object to update.
         */
        void flush(Account *);

        /**
         * Gets the value of a quest variable.
         *
         * @param id character id.
         * @param name quest var name to get.
         */
        std::string getQuestVar(int id, const std::string &);

        /**
         * Sets the value of a quest variable.
         *
         * @param id character id.
         * @param name quest var name to set.
         * @param value value to set.
         */
        void setQuestVar(int id, const std::string &, const std::string &);

        /**
         * Gets the string value of a map specific world state variable.
         *
         * @param name Name of the requested world-state variable.
         * @param map_id Id of the specific map.
         */
        std::string getWorldStateVar(const std::string &name, int mapId = -1);

        /**
         * Sets the value of a world state variable.
         *
         * @param name Name of the world-state vairable.
         * @param value New value of the world-state variable.
         */
        void setWorldStateVar(const std::string &name,
                              const std::string &value);

        /**
         * Sets the value of a world state variable of a specific map.
         *
         * @param name Name of the world-state vairable.
         * @param mapId ID of the specific map
         * @param value New value of the world-state variable.
         */
        void setWorldStateVar(const std::string &name, int mapId,
                              const std::string &value);

        /**
         * Set the level on an account.
         *
         * @param id The id of the account
         * @param level The level to set for the account
         */
        void setAccountLevel(int id, int level);

        /**
         * Set the level on a character.
         *
         * @param id The id of the character
         * @param level The level to set for the character
         */
        void setPlayerLevel(int id, int level);

        /**
         * Store letter.
         *
         * @param letter The letter to store
         */
        void storeLetter(Letter *letter);

        /**
         * Retrieve post
         *
         * @param playerId The id of the character requesting his post
         */
        Post *getStoredPost(int playerId);

        /**
         * Delete a letter from the database.
         * @param letter The letter to delete.
         */
        void deletePost(Letter *letter);

        /**
         * Returns the version of the local item database.
         *
         * @return the database version number.
         */
        unsigned int getItemDatabaseVersion() const
        { return mItemDbVersion; }

        /**
         * Sets the status of a character to online (true) or offline (false).
         *
         * @param charId Id of the character.
         * @param online True to mark the character as being online.
         */
        void setOnlineStatus(int charId, bool online);

        /**
         * Store a transaction.
         *
         * @param trans The transaction to add in the logs.
         */
        void addTransaction(const Transaction &trans);

        /**
         * Retrieve the last \a num transactions that were stored.
         *
         * @return a vector of transactions.
         */
        std::vector<Transaction> getTransactions(unsigned int num);

        /**
         * Retrieve all transactions since the given \a date.
         *
         * @return a vector of transactions.
         */
        std::vector<Transaction> getTransactions(time_t date);

        /**
         * Provides direct access to the database. Use with care!
         *
         * @return a database provider object.
         */
        dal::DataProvider *database() const
        { return mDb; }

    private:
        // Prevent copying
        Storage(const Storage &rhs);
        Storage &operator=(const Storage &rhs);

        /**
         * Gets an account from a prepared SQL statement
         *
         * @return the account found
         */
        Account *getAccountBySQL();

        /**
         * Gets a character from a prepared SQL statement
         *
         * @param owner the account the character is in.
         *
         * @return the character found by the query.
         */
        Character *getCharacterBySQL(Account *owner);

        /**
         * Synchronizes the base data in the connected SQL database with the xml
         * files like items.xml.
         * This method is called once after initialization of DALStorage.
         * Probably this function should be called if a gm requests an online
         * reload of the xml files to load new items or monsters without server
         * restart.
         */
        void syncDatabase();

        dal::DataProvider *mDb;         /**< the data provider */
        unsigned int mItemDbVersion;    /**< Version of the item database. */
};

extern Storage *storage;

#endif // STORAGE_H
