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


#include "object.h"
#include "account.h"


namespace tmwserv
{


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
         * Create an instance of Storage.
         *
         * @return the unique instance of Storage.
         *
         * @exception std::bad_alloc if the instance cannot be created.
         */
        static Storage&
        instance(void);


        /**
         * Delete the storage.
         */
        static void
        destroy(void);


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
         * Make sure any changes are saved.
         */
        virtual void
        flush(void) = 0;


        /**
         * Account count (test function).
         */
        virtual unsigned int
        getAccountCount(void) = 0;


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


    private:
        static Storage* mInstance; /**< the unique instance of Storage */
};


} // namespace tmwserv


#endif // _TMWSERV_STORAGE_H_
