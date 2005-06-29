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


#include <map>

#include "account.h"


namespace tmwserv
{


/**
 * Enumeration type for the account status:
 *
 * AS_NEW_ACCOUNT  : set to an account that is added to the storage and
 *                   hence not yet save to disk (file or database);
 *                   accounts with this status will be saved to disk at
 *                   the next flush.
 * AS_ACC_TO_UPDATE: set to an account that was loaded from disk;
 *                   accounts with this status will be updated at the next
 *                   flush.
 * AS_ACC_TO_DELETE: set to an account that was loaded from disk;
 *                   accounts with this status will be deleted from disk at
 *                   the next flush.
 *
 * Notes: an account that is requested to be deleted and still has the
 *        status AS_NEW_ACCOUNT (and therefore not yet saved to disk) will
 *        be immediately deleted from memory to save useless transactions
 *        to disk.
 */
typedef enum {
    AS_NEW_ACCOUNT,
    AS_ACC_TO_UPDATE,
    AS_ACC_TO_DELETE
} AccountStatus;


/**
 * Structure type for the account info.
 */
typedef struct {
    AccountStatus status;
    unsigned int id;
} AccountInfo;


/**
 * Functor to be used as the sorting criterion of the map defined below.
 */
struct account_sort_by_name
    : public std::binary_function<Account*, Account*, bool>
{
    bool
    operator()(Account* acc1, Account* acc2) const
    {
        return (acc1->getName() < acc2->getName());
    }
};


/**
 * Data type for the list of accounts.
 *
 * Notes:
 *     - the account id is not attribute of Account but AccountInfo because
 *       only the storage should modify this value, this attribute is for
 *       internal use.
 */
typedef std::map<Account*, AccountInfo, account_sort_by_name> Accounts;


/**
 * Functor used to search an Account by name in the map defined above.
 */
class account_by_name
{
    public:
        /**
         * Constructor.
         */
        account_by_name(const std::string& name)
            : mName(name)
        {
            // NOP
        }


        /**
         * Operator().
         */
        bool
        operator()(std::pair<Account*, AccountInfo> elem) const
        {
            return (elem.first)->getName() == mName;
        }


    private:
        std::string mName; /**< the name to look for */
};


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
        destroy(void);


        /**
         * Open the storage for read/write access.
         *
         * Depending on the underlying implementation of Storage, opening
         * a storage would mean either opening a file or connecting to a
         * database.
         */
        virtual void
        open(void) = 0;


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
        bool
        isOpen(void) const;


        /**
         * Get the storage name.
         *
         * @return the storage name.
         */
        const std::string&
        getName(void) const;


        /**
         * Set a user name for the storage.
         *
         * Depending on the underlying implementation of Storage, setting
         * the user name may have no effect (e.g. DALStorage running on
         * SQLite).
         *
         * @param userName the user name.
         */
        void
        setUser(const std::string& userName);


        /**
         * Get the user name.
         *
         * @return the user name (it may be an empty string if not set
         *         previously).
         */
        const std::string&
        getUser(void) const;


        /**
         * Set a user password for the storage.
         *
         * Depending on the underlying implementation of Storage, setting
         * the user password may have no effect (e.g. DALStorage running on
         * SQLite).
         *
         * @param password the user password.
         */
        void
        setPassword(const std::string& password);


        /**
         * Get the user password.
         *
         * @return the user password (it may be an empty string if not set
         *         previously).
         */
        const std::string&
        getPassword(void) const;


        /**
         * Get an account by user name.
         *
         * @param userName the owner of the account.
         *
         * @return the account associated to the user name.
         */
        virtual Account*
        getAccount(const std::string& userName) = 0;


        /**
         * Add a new account.
         *
         * @param account the new account.
         */
        virtual void
        addAccount(const Account* account) = 0;


        /**
         * Delete an account.
         *
         * @param userName the owner of the account.
         */
        virtual void
        delAccount(const std::string& userName) = 0;


        /**
         * Saves the changes permanently.
         */
        virtual void
        flush(void) = 0;


    protected:
        /**
         * Default constructor.
         */
        Storage(void)
            throw();


        /**
         * Destructor.
         */
        virtual
        ~Storage(void)
            throw();


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
        Beings mCharacters; /**< the loaded characters */
        bool mIsOpen;       /**< flag is true if the storage is open */


    private:
        static Storage* mInstance;    /**< the unique instance of Storage */
        static std::string mName;     /**< the name of the storage */
        static std::string mUser;     /**< the user name */
        static std::string mPassword; /**< the user password */
};


} // namespace tmwserv


#endif // _TMWSERV_STORAGE_H_
