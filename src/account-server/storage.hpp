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


#ifndef _TMWSERV_STORAGE_H_
#define _TMWSERV_STORAGE_H_

#include <list>
#include <map>

#include "account-server/account.hpp"
#include "account-server/characterdata.hpp"
#include "account-server/guild.hpp"
#include "chat-server/chatchannel.hpp"

/**
 * Data type for the list of accounts.
 */
typedef std::map<unsigned, AccountPtr> Accounts;


/**
 * A storage to load and persist dynamic data.
 *
 * Notes:
 *     - this class implements the singleton design pattern.
 *     - destroy() must be called at least once before the application
 *       exits or else there will be a memory leak.
 */
class Storage
{
    public:
        /**
         * Create a named instance of Storage.
         *
         * @param name the name of the storage.
         *
         * @return the unique instance of Storage.
         *
         * @exception std::bad_alloc if the instance cannot be created.
         *
         * Notes:
         *     - it is up to the underlying implementation of Storage to
         *       decide about what to do with the name, it could serve as the
         *       name of the database or the name of the file into which the
         *       storage will be dumped to.
         *     - the name of the storage is saved only when it's the first
         *       invocation of instance() or only when instance() is invoked
         *       after destroy().
         */
        static Storage&
        instance(const std::string& name);


        /**
         * Delete the storage.
         */
        static void
        destroy();


        /**
         * Open the storage for read/write access.
         *
         * Depending on the underlying implementation of Storage, opening
         * a storage would mean either opening a file or connecting to a
         * database.
         */
        virtual void
        open() = 0;


        /**
         * Close the storage.
         *
         * Depending on the underlying implementation of Storage, closing
         * a storage would mean either closing a file or disconnecting from
         * a database.
         */
        virtual void
        close(void) = 0;


        /**
         * Check if the storage is open.
         *
         * @return true if the storage is open.
         */
        bool isOpen() const
        { return mIsOpen; }


        /**
         * Get the storage name.
         *
         * @return the storage name.
         */
        std::string const &getName() const
        { return mName; }


        /**
         * Set a user name for the storage.
         *
         * Depending on the underlying implementation of Storage, setting
         * the user name may have no effect (e.g. DALStorage running on
         * SQLite).
         *
         * @param userName the user name.
         */
        void setUser(const std::string& userName)
        { mUser = userName; }


        /**
         * Get the user name.
         *
         * @return the user name (it may be an empty string if not set
         *         previously).
         */
        std::string const &getUser() const
        { return mUser; }


        /**
         * Set a user password for the storage.
         *
         * Depending on the underlying implementation of Storage, setting
         * the user password may have no effect (e.g. DALStorage running on
         * SQLite).
         *
         * @param password the user password.
         */
        void setPassword(const std::string& password)
        { mPassword = password; }


        /**
         * Get the user password.
         *
         * @return the user password (it may be an empty string if not set
         *         previously).
         */
        std::string const &getPassword() const
        { return mPassword; }


        /**
         * Get an account by user name.
         *
         * @param userName the owner of the account.
         *
         * @return the account associated to the user name.
         */
        virtual AccountPtr
        getAccount(const std::string& userName) = 0;

        /**
         * Get an account by ID.
         *
         * @param accountID the ID of the account.
         *
         * @return the account associated with the ID.
         */
        virtual AccountPtr
        getAccountByID(int accountID) = 0;

        /**
         * Gets a character by database ID.
         *
         * @param id the ID of the character.
         *
         * @return the character associated to the ID.
         */
        virtual CharacterPtr getCharacter(int id) = 0;
        
        /**
         * Gets a character by name.
         *
         * @param name the name of the character.
         *
         * @return the character associated to the name.
         */
        virtual CharacterPtr getCharacter(const std::string &name) = 0;

        /**
         * Add a new account.
         *
         * @param account the new account.
         */
        virtual void
        addAccount(const AccountPtr& account) = 0;

        /**
         * Delete an account.
         *
         * @param account the account to delete.
         */
        virtual void
        delAccount(AccountPtr const &account) = 0;

        /**
         * Flush and unload an account.
         *
         * @param account the account to unload.
         */
        virtual void
        unloadAccount(AccountPtr const &account) = 0;

        /**
         * Get the list of Emails in the accounts list.
         * @return the list of Email's Addresses.
         *
         * @deprecated The only purpose of using this list inside the server is
         *             for checking for existing email addresses, which is
         *             covered by Storage::getSameEmailNumber().
         *             It could later be used for mailing list announcement.
         */
        virtual
        std::list<std::string> getEmailList() = 0;

        /**
         * Tells if the email address already exists.
         * @return true if the email address exists.
         */
        virtual bool doesEmailAddressExist(std::string const &email) = 0;

        /**
         * Tells if the character's name already exists
         * @return true if character's name exists.
         */
        virtual bool doesCharacterNameExist(std::string const &name) = 0;

        /**
         * Updates the data for a single character, does not update the
         * owning account or the characters name.
         * Primary usage should be storing characterdata received from a
         * game server.
         * returns true if succefull, false otherwise.
         */
        virtual bool
        updateCharacter(CharacterPtr ptr) = 0;

        /**
         * Gives the list of opened public channels registered in database
         * @return a map of the public channels
         */
        virtual std::map<unsigned short, ChatChannel>
        getChannelList() = 0;

        /**
         * apply channel differences from the list in memory
         * to the one in db.
         */
        virtual void
        updateChannels(std::map<unsigned short, ChatChannel>& channelList) = 0;

        /**
         * Add a new guild
         * 
         */
        virtual void
        addGuild(Guild* guild) = 0;
        
        /**
         * Delete a guild
         *
         */
        virtual void
        removeGuild(Guild* guild) = 0;
        
        /**
         * Add member to guild
         *
         */
        virtual void
        addGuildMember(int guild_id, const std::string &member_name) = 0;
        
        /**
         * Remove member from guild
         */
        virtual void
        removeGuildMember(int guildId, const std::string &memberName) = 0;
        
        /**
         * Get guild list
         *@return a list of guilds
         *
         */
        virtual std::list<Guild*>
        getGuildList() = 0;
        
        /**
         * Saves the changes to all the accounts permanently.
         */
        virtual void flushAll() = 0;

        /**
         * Saves the changes to one account permanently.
         */
        virtual void flush(AccountPtr const &account) = 0;

        /**
         * Gets the value of a quest variable.
         */
        virtual std::string getQuestVar(int id, std::string const &) = 0;

        /**
         * Sets the value of a quest variable.
         */
        virtual void setQuestVar(int id, std::string const &,
                                 std::string const &) = 0;


    protected:
        /**
         * Default constructor.
         */
        Storage() {}


        /**
         * Destructor.
         */
        virtual ~Storage() {}


        /**
         * Copy constructor.
         */
        Storage(const Storage& rhs);


        /**
         * Assignment operator.
         */
        Storage&
        operator=(const Storage& rhs);


    protected:
        Accounts mAccounts; /**< list of accounts in memory */
        Characters mCharacters; /**< the loaded characters */
        bool mIsOpen;       /**< flag is true if the storage is open */


    private:
        static Storage* mInstance;    /**< the unique instance of Storage */
        static std::string mName;     /**< the name of the storage */
        static std::string mUser;     /**< the user name */
        static std::string mPassword; /**< the user password */
};

#endif // _TMWSERV_STORAGE_H_
