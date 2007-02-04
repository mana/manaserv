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


#ifndef _TMWSERV_DALSTORAGE_H_
#define _TMWSERV_DALSTORAGE_H_

#include "account-server/storage.hpp"
#include "dal/dataprovider.h"

/**
 * A storage class that relies on DAL.
 *
 * Notes:
 *     - this class cannot be instanciated nor duplicated in order to force
 *       a user class to use the Storage singleton.
 */
class DALStorage: public Storage
{
    // friend so that Storage can call the constructor.
    friend class Storage;


    public:
        /**
         * Connect to the database and initialize it if necessary.
         */
        void
        open(void);


        /**
         * Disconnect from the database.
         */
        void
        close(void);


        /**
         * Get an account by user name.
         *
         * @param userName the owner of the account.
         *
         * @return the account associated to the user name.
         */
        AccountPtr
        getAccount(const std::string& userName);

        /**
         * Get an account by ID.
         *
         * @param accountID the ID of the account.
         *
         * @return the account associated with the ID.
         */
        AccountPtr
        getAccountByID(int accountID);

        /**
         * Gets a character by database ID.
         *
         * @param id the ID of the character.
         *
         * @return the character associated to the ID.
         */
        PlayerPtr getCharacter(int id);

        /**
         * Add a new account.
         *
         * @param account the new account.
         */
        void
        addAccount(const AccountPtr& account);


        /**
         * Delete an account.
         *
         * @param account the account to delete.
         */
        void delAccount(AccountPtr const &account);

        /**
         * Flush and unload an account.
         *
         * @param account the account to unload.
         */
        void unloadAccount(AccountPtr const &account);

        /**
         * Get the list of Emails in the accounts list.
         * @return the list of Email's Addresses.
         */
        std::list<std::string>
        getEmailList();

        /**
         * Tells if the email address already exists.
         * @return true if the email address exists.
         */
        bool doesEmailAddressExist(std::string const &email);

        /**
         * Tells if the character's name already exists
         * @return true if character's name exists.
         */
        bool doesCharacterNameExist(std::string const &name);

        /**
         * Gives the list of opened public channels registered in database
         * @return a map of the public channels
         */
        std::map<short, ChatChannel>
        getChannelList();

        /**
         * apply channel differences from the list in memory
         * to the one in db.
         */
        void
        updateChannels(std::map<short, ChatChannel>& channelList);

        /**
         * Save changes to the database permanently.
         *
         * @exception tmwserv::dal::DbSqlQueryExecFailure.
         */
        void flushAll();
        void flush(AccountPtr const &);

    private:
        /**
         * Constructor.
         */
        DALStorage(void);


        /**
         * Destructor.
         */
        ~DALStorage(void)
            throw();


        /**
         * Copy constructor.
         */
        DALStorage(const DALStorage& rhs);


        /**
         * Assignment operator.
         */
        DALStorage&
        operator=(const DALStorage& rhs);


        /**
         * Create the specified table.
         *
         * @param tblName the table name.
         * @param sql the SQL query to execute.
         *
         * @exception tmwserv::dal::DbSqlQueryExecFailure.
         */
        void
        createTable(const std::string& tblName,
                    const std::string& sql);


    private:
        std::auto_ptr<dal::DataProvider> mDb; /**< the data provider */
};

#endif // _TMWSERV_DALSTORAGE_H_
