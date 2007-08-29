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

#include <cassert>

#include "account-server/account.hpp"

#include "account-server/accountclient.hpp"
#include "utils/functors.h"

/**
 * Constructor with initial account info.
 */
Account::Account(const std::string& name,
                 const std::string& password,
                 const std::string& email,
                 int level,
                 int id)
        : mName(name),
          mPassword(password),
          mEmail(email),
          mCharacters(),
          mID(id),
          mLevel(level)
{
    // NOOP
}


/**
 * Destructor.
 */
Account::~Account()
{
    // mCharacters is a list of smart pointers which will take care about
    // deallocating the memory so nothing to deallocate here :)
}


/**
 * Set the characters.
 */
void
Account::setCharacters(const Characters& characters)
{
    mCharacters = characters;
}


/**
 * Add a new character.
 */
void
Account::addCharacter(CharacterPtr character)
{
    if (character.get() != 0) {
        mCharacters.push_back(character);
    }
}

/**
 * Remove a character.
 */
bool Account::delCharacter(std::string const &name)
{
    Characters::iterator
        end = mCharacters.end(),
        it = std::find_if(mCharacters.begin(), end,
                          std::bind2nd(obj_name_is<CharacterPtr>(), name));

    if (it == end) return false;
    mCharacters.erase(it);
    return true;
}


/**
 * Get a character by name.
 */
CharacterPtr Account::getCharacter(const std::string& name)
{
    Characters::iterator
        end = mCharacters.end(),
        it = std::find_if(mCharacters.begin(), end,
                          std::bind2nd(obj_name_is<CharacterPtr>(), name));

    if (it != end) return *it;
    return CharacterPtr();
}

void Account::setID(int id)
{
    assert(mID < 0);
    mID = id;
}
