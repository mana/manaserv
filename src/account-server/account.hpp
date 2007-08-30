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

#include "account-server/character.hpp"

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
 * A player's account.
 */
class Account
{
    public:
        /**
         * Constructor.
         */
        Account(int id = -1): mID(id)
        {}

        /**
         * Destructor.
         */
        ~Account();


        /**
         * Set the user name.
         *
         * @param name the user name.
         */
        void setName(std::string const &name)
        { mName = name; }


        /**
         * Get the user name.
         *
         * @return the user name.
         */
        std::string const &getName() const
        { return mName; }


        /**
         * Set the user password.
         *
         * @param password the user password.
         */
        void setPassword(std::string const &password)
        { mPassword = password; }


        /**
         * Get the user password.
         *
         * @return the user password.
         */
        std::string const &getPassword() const
        { return mPassword; }


        /**
         * Set the user email address.
         *
         * @param email the user email address.
         */
        void setEmail(std::string const &email)
        { mEmail = email; }


        /**
         * Get the user email address.
         *
         * @return the user email address.
         */
        std::string const &getEmail() const
        { return mEmail; }


        /**
         * Set the account level.
         *
         * @param level the new level.
         */
        void setLevel(int level)
        { mLevel = level; }


        /**
         * Get the account level.
         *
         * @return the account level.
         */
        int getLevel() const
        { return mLevel; }


        /**
         * Set the characters.
         *
         * @param characters a list of characters.
         */
        void
        setCharacters(const Characters& characters);


        /**
         * Add a new character.
         *
         * @param character the new character.
         */
        void addCharacter(Character *character);

        /**
         * Remove a character.
         *
         * @param name The character's name to delete.
         */
        bool
        delCharacter(std::string const &name);


        /**
         * Get all the characters.
         *
         * @return all the characters.
         */
        Characters &getCharacters()
        { return mCharacters; }

        /**
         * Get all the characters.
         *
         * @return all the characters.
         */
        Characters const &getCharacters() const
        { return mCharacters; }

        /**
         * Get account ID.
         *
         * @return the unique ID of the account, a negative number if none yet.
         */
         int getID() const
         { return mID; }

        /**
         * Set account ID.
         * The account shall not have any ID yet.
         */
         void setID(int);

    private:
        Account(Account const &rhs);
        Account &operator=(Account const &rhs);


    private:
        std::string mName;     /**< user name */
        std::string mPassword; /**< user password (encrypted) */
        std::string mEmail;    /**< user email address */
        Characters mCharacters;   /**< Character data */
        int mID;               /**< unique id */
        unsigned char mLevel;   /**< account level */
};

typedef std::vector< Account * > Accounts;

#endif // _TMWSERV_ACCOUNT_H_
