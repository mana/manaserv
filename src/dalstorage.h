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


#include <vector>

#include "dal/dataproviderfactory.h"
#include "dalstoragesql.h"
#include "storage.h"


namespace tmwserv
{


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
        Account*
        getAccount(const std::string& userName);


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
         * @param userName the owner of the account.
         */
        void
        delAccount(const std::string& userName);

        /**
         * Get the list of Emails in the accounts list.
         * @return the list of Email's Addresses.
         */
        std::list<std::string>
        getEmailList();

        /**
         * Tells if Email already exists.
         * @return true if Email is already in database
         */
        bool
        doesEmailAlreadyExists(std::string email);

        /**
         * Tells if the character's name already exists
         * @return true if character's name exists.
         */
        bool
        doesCharacterNameExists(std::string name);

        /**
         * Save changes to the database permanently.
         *
         * @exception tmwserv::dal::DbSqlQueryExecFailure.
         */
        void
        flush(void);


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


        /**
         * Add an account to the database.
         *
         * @param account the account to add.
         *
         * @exeception tmwserv::dal::DbSqlQueryExecFailure.
         */
        void
        _addAccount(const AccountPtr& account);


        /**
         * Update an account from the database.
         *
         * @param account the account to update.
         *
         * @exception tmwserv::dal::DbSqlQueryExecFailure.
         */
        void
        _updAccount(const AccountPtr& account);


        /**
         * Delete an account and its associated data from the database.
         *
         * @param account the account to update.
         *
         * @exception tmwserv::dal::DbSqlQueryExecFailure.
         */
        void
        _delAccount(const AccountPtr& account);


        /**
         * Delete an account and its associated data from the database.
         *
         * @param userName the owner of the account.
         *
         * @exeception tmwserv::dal::DbSqlQueryExecFailure.
         */
        void
        _delAccount(const std::string& userName);


    private:
        std::auto_ptr<dal::DataProvider> mDb; /**< the data provider */
};


} // namespace tmwserv


#endif // _TMWSERV_DALSTORAGE_H_
