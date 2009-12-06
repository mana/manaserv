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

#ifndef DALSTORAGE_H
#define DALSTORAGE_H

#include <list>
#include <map>
#include <vector>

#include "../dal/dataprovider.h"

#include "../common/transaction.hpp"

class Account;
class Character;
class ChatChannel;
class Guild;
class Letter;
class Post;

/**
 * A storage class that relies on DAL.
 */
class DALStorage
{
    public:
        /**
         * Constructor.
         */
        DALStorage();

        /**
         * Destructor.
         */
        ~DALStorage();

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
        Account *getAccount(const std::string& userName);

        /**
         * Get an account by ID.
         *
         * @param accountID the ID of the account.
         *
         * @return the account associated with the ID.
         */
        Account *getAccount(int accountID);

        /**
         * Gets a character by database ID.
         *
         * @param id the ID of the character.
         * @param owner the account the character is in.
         *
         * @return the character associated to the ID.
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
         * Add a new account.
         *
         * @param account the new account.
         */
        void addAccount(Account *account);

        /**
         * Delete an account.
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
         * @param AttribId    ID of the modified attribute
         * @param AttribValue New value of the modified attribute
         */
        void updateCharacterPoints(int charId,
                                   int charPoints, int corrPoints,
                                   int attribId, int attribValue);

        /**
         * Write a modification message about character skills to the database.
         * @param CharId      ID of the character
         * @param SkillId     ID of the skill
         * @param SkillValue  new skill points
         */
        void updateExperience(int charId, int skillId, int skillValue);

        /**
         * Inserts a record about a status effect into the database
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
         * @param startTransaction indicates wheter the function should run in
         *        its own transaction or is called inline of another transaction
         */
        void delCharacter(int charId, bool startTransaction) const;

        /**
         * Delete a character in the database. The object itself i not touched
         * by this function!
         *
         * @param character character object.
         * @param startTransaction indicates wheter the function should run in
         *        its own transaction or is called inline of another transaction
         */
        void delCharacter(Character *character, bool startTransaction) const;

        /**
         * Removes expired bans from accounts
         */
        void checkBannedAccounts();

        /**
         * Tells if the user name already exists.
         * @return true if the user name exists.
         */
        bool doesUserNameExist(const std::string &name);

        /**
         * Tells if the email address already exists.
         * @return true if the email address exists.
         */
        bool doesEmailAddressExist(const std::string &email);

        /**
         * Tells if the character name already exists.
         * @return true if the character name exists.
         */
        bool doesCharacterNameExist(const std::string &name);

        /**
         * Updates the data for a single character, does not update the
         * owning account or the characters name.
         * Primary usage should be storing characterdata received from a
         * game server.
         * returns true if succefull, false otherwise.
         * @param ptr Character to store values in the database.
         * @param startTransaction set to false if this method is called as
         *                         nested transaction.
         * @return true on success
         */
        bool updateCharacter(Character *ptr,
                             bool startTransaction = true);

        /**
         * Save changes of a skill to the database permanently.
         *
         * @param character Character thats skill has changed.
         * @param skill_id Identifier of the changed skill.
         *
         * @exception dbl::DbSqlQueryExecFailure.
         */
        void flushSkill(const Character *character, int skill_id);

        /**
         * Add a new guild.
         */
        void addGuild(Guild *guild);

        /**
         * Delete a guild.
         */
        void removeGuild(Guild *guild);

        /**
         * Add member to guild.
         */
        void addGuildMember(int guild_id, int memberId);

        /**
         * Remove member from guild.
         */
        void removeGuildMember(int guildId, int memberId);

        /**
         * Save guild member rights.
         */
        void setMemberRights(int guildId, int memberId, int rights);

        /**
         * Get guild list.
         * @return a list of guilds
         */
        std::list<Guild*> getGuildList();

        /**
         * Save changes to the database permanently.
         *
         * @exception dal::DbSqlQueryExecFailure.
         */
        void flushAll();
        void flush(Account *);

        /**
         * Gets the value of a quest variable.
         */
        std::string getQuestVar(int id, const std::string &);

        /**
         * Sets the value of a quest variable.
         */
        void setQuestVar(int id, const std::string &, const std::string &);

        /**
         * Gets the string value of a map specific world state variable.
         *
         * @param name Name of the requested world-state variable.
         * @param map_id Id of the specific map.
         */
        std::string getWorldStateVar(const std::string &name, int map_id = -1);

        /**
         * Sets the value of a world state variable.
         *
         * @param name Name of the world-state vairable.
         * @param value New value of the world-state variable.
         */
        void setWorldStateVar(const std::string &name, const std::string &value);

        /**
         * Sets the value of a world state variable of a specific map.
         *
         * @param name Name of the world-state vairable.
         * @param map_id ID of the specific map
         * @param value New value of the world-state variable.
         */
        void setWorldStateVar(const std::string &name, int map_id,
                              const std::string &value);

        /**
         * Set the level on an account
         *
         * @param id The id of the account
         * @param level The level to set for the account
         */
        void setAccountLevel(int id, int level);

        /**
         * Set the level on a character
         *
         * @param id The id of the character
         * @param level The level to set for the character
         */
        void setPlayerLevel(int id, int level);

        /**
         * Store letter
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
        void deletePost(Letter* letter);

        /**
         * Add item to auction
         *
         * @param itemId The id of the item for auction
         * @param player The player who put the item up for auction
         * @param gold The amount of money to buy it
         */
        void addAuctionItem(unsigned int itemId, int playerId, unsigned int gold);

        /**
         * Gets the version of the local item database.
         *
         * @return Version of the item database.
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
         * Store a transaction
         */
        void addTransaction(const Transaction &trans);

        /**
         * Retrieve a series of transactions
         * Either based on number of transactions last saved
         * or by all transactions since a date
         */
        std::vector<Transaction> getTransactions(unsigned int num);
        std::vector<Transaction> getTransactions(time_t date);

    private:
        /**
         * Copy constructor.
         */
        DALStorage(const DALStorage &rhs);


        /**
         * Assignment operator.
         */
        DALStorage &operator=(const DALStorage &rhs);

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

        dal::DataProvider *mDb; /**< the data provider */
        unsigned int mItemDbVersion;    /**< Version of the item database. */
};

extern DALStorage *storage;

#endif // DALSTORAGE_H
