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


#include "utils/cipher.h"
#include "utils/functors.h"

#include "account.h"


namespace tmwserv
{


/**
 * Default constructor.
 */
Account::Account(void)
    throw()
{
    // NOOP
}


/**
 * Constructor with initial account info.
 */
Account::Account(const std::string& name,
                 const std::string& password,
                 const std::string& email,
                 const Beings& characters)
        : mName(name),
          mEmail(email),
          mCharacters(characters)
{
    // password must be encrypted.
    setPassword(password);
}


/**
 * Destructor.
 */
Account::~Account(void)
    throw()
{
    // we do not delete the Beings here because Storage keeps
    // a list of Beings as well and the destructor of Storage
    // takes care about it.

    /*
     *for (Beings::iterator it = mCharacters.begin();
     *     it != mCharacters.end();
     *     ++it)
     *{
     *    delete (*it);
     *}
     */
}


/**
 * Set the user name.
 */
void
Account::setName(const std::string& name)
{
    mName = name;
}


/**
 * Get the user name.
 */
const std::string&
Account::getName(void) const
{
    return mName;
}


/**
 * Set the user password.
 */
void
Account::setPassword(const std::string& password)
{
    // use MD5 digest algorithm to cipher the password.
    mPassword = utils::Cipher::instance().md5(password);
}


/**
 * Get the user password.
 */
const std::string&
Account::getPassword(void) const
{
    return mPassword;
}


/**
 * Set the user email address.
 */
void
Account::setEmail(const std::string& email)
{
    // should we check that the email address is valid first?
    mEmail = email;
}


/**
 * Get the user email address.
 */
const std::string&
Account::getEmail(void) const
{
    return mEmail;
}


/**
 * Set the characters.
 */
void
Account::setCharacters(const Beings& characters)
{
    mCharacters = characters;
}


/**
 * Add a new character.
 */
void
Account::addCharacter(Being* character)
{
    if (character) {
        mCharacters.push_back(character);
    }
}


/**
 * Get all the characters.
 */
Beings&
Account::getCharacters(void)
{
    return mCharacters;
}


/**
 * Get a character by name.
 */
Being*
Account::getCharacter(const std::string& name)
{
    Beings::iterator it =
        std::find_if(mCharacters.begin(),
                     mCharacters.end(),
                     std::bind2nd(obj_name_is<Being*>(), name)
                    );

    if (it != mCharacters.end()) {
        return (*it);
    }

    return 0;
}


} // namespace tmwserv
