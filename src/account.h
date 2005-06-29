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


#ifndef _TMWSERV_ACCOUNT_H_
#define _TMWSERV_ACCOUNT_H_


#include <string>
#include <vector>

#include "being.h"


namespace tmwserv
{


/**
 * Notes:
 *     - change from the previous implementation: this class does not encrypt
 *       passwords anymore and will just store the passwords as they are
 *       passed to setPassword().
 *     - the encryption should and must be performed externally from this
 *       class or else we would end up with the password being encrypted many
 *       times (e.g setPassword(getPassword()) would encrypt the password
 *       twice or setPassword(encrypted_password_from_database) would also
 *       encrypt the password twice).
 */


/**
 * A player account.
 */
class Account
{
    public:
        /**
         * Constructor with initial account info.
         *
         * @param name the user name.
         * @param password the user password.
         * @param email the user email.
         */
        Account(const std::string& name,
                const std::string& password,
                const std::string& email);


        /**
         * Constructor with initial account info.
         *
         * @param name the user name.
         * @param password the user password.
         * @param email the user email.
         * @param characters the characters.
         */
        Account(const std::string& name,
                const std::string& password,
                const std::string& email,
                const Beings& characters);


        /**
         * Destructor.
         */
        ~Account(void)
            throw();


        /**
         * Set the user name.
         *
         * @param name the user name.
         */
        void
        setName(const std::string& name);


        /**
         * Get the user name.
         *
         * @return the user name.
         */
        const std::string&
        getName(void) const;


        /**
         * Set the user password.
         *
         * @param password the user password.
         */
        void
        setPassword(const std::string& password);


        /**
         * Get the user password.
         *
         * @return the user password.
         */
        const std::string
        getPassword(void) const;


        /**
         * Set the user email address.
         *
         * @param email the user email address.
         */
        void
        setEmail(const std::string& email);


        /**
         * Get the user email address.
         *
         * @return the user email address.
         */
        const std::string&
        getEmail(void) const;


        /**
         * Set the account level.
         *
         * @param level the new level.
         */
        void
        setLevel(const AccountLevels level);


        /**
         * Get the account level.
         *
         * @return the account level.
         */
        AccountLevels
        getLevel(void) const;


        /**
         * Set the characters.
         *
         * @param characters a list of characters.
         */
        void
        setCharacters(const Beings& characters);


        /**
         * Add a new character.
         *
         * @param character the new character.
         */
        void
        addCharacter(Being* character);


        /**
         * Get all the characters.
         *
         * @return all the characters.
         */
        Beings&
        getCharacters(void);


        /**
         * Get a character by name.
         *
         * @return the character if found, NULL otherwise.
         */
        Being*
        getCharacter(const std::string& name);


    private:
        /**
         * Default constructor.
         */
        Account(void)
            throw();


        /**
         * Copy constructor.
         */
        Account(const Account& rhs);


        /**
         * Assignment operator.
         */
        Account&
        operator=(const Account& rhs);


    private:
        std::string mName;     /**< user name */
        std::string mPassword; /**< user password (encrypted) */
        std::string mEmail;    /**< user email address */
        Beings mCharacters;    /**< player data */
        AccountLevels mLevel;  /**< account level */
};


} // namespace tmwserv


#endif // _TMWSERV_ACCOUNT_H_
