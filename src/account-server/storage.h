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

        void open();
        void close();

        Account *getAccount(const std::string &userName);
        Account *getAccount(int accountID);

        Character *getCharacter(int id, Account *owner);
        Character *getCharacter(const std::string &name);

        void addAccount(Account *account);
        void delAccount(Account *account);

        void updateLastLogin(const Account *account);

        void updateCharacterPoints(int charId,
                                   int charPoints, int corrPoints);

        void updateExperience(int charId, int skillId, int skillValue);

        void updateAttribute(int charId, unsigned int attrId,
                             double base, double mod);

        void updateKillCount(int charId, int monsterId, int kills);

        void insertStatusEffect(int charId, int statusId, int time);

        void banCharacter(int id, int duration);

        void delCharacter(int charId) const;
        void delCharacter(Character *character) const;

        void checkBannedAccounts();

        bool doesUserNameExist(const std::string &name);
        bool doesEmailAddressExist(const std::string &email);
        bool doesCharacterNameExist(const std::string &name);

        bool updateCharacter(Character *ptr);

        void flushSkill(const Character *character, int skill_id);

        void addGuild(Guild *guild);
        void removeGuild(Guild *guild);

        void addGuildMember(int guild_id, int memberId);
        void removeGuildMember(int guildId, int memberId);

        void setMemberRights(int guildId, int memberId, int rights);

        std::list<Guild*> getGuildList();

        void flush(Account *);

        std::string getQuestVar(int id, const std::string &);
        void setQuestVar(int id, const std::string &, const std::string &);

        std::string getWorldStateVar(const std::string &name, int map_id = -1);
        void setWorldStateVar(const std::string &name,
                              const std::string &value);
        void setWorldStateVar(const std::string &name, int mapId,
                              const std::string &value);

        void setAccountLevel(int id, int level);
        void setPlayerLevel(int id, int level);

        void storeLetter(Letter *letter);
        Post *getStoredPost(int playerId);
        void deletePost(Letter *letter);

        /**
         * Returns the version of the local item database.
         */
        unsigned int getItemDatabaseVersion() const
        { return mItemDbVersion; }

        void setOnlineStatus(int charId, bool online);

        void addTransaction(const Transaction &trans);

        std::vector<Transaction> getTransactions(unsigned int num);
        std::vector<Transaction> getTransactions(time_t date);

        /**
         * Provides direct access to the database. Use with care!
         */
        dal::DataProvider *database() const { return mDb; }

    private:
        // Prevent copying
        Storage(const Storage &rhs);
        Storage &operator=(const Storage &rhs);

        Account *getAccountBySQL();
        Character *getCharacterBySQL(Account *owner);

        void syncDatabase();

        dal::DataProvider *mDb; /**< the data provider */
        unsigned int mItemDbVersion;    /**< Version of the item database. */
};

extern Storage *storage;

#endif // STORAGE_H
