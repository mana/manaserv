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

/**
 * Destructor.
 */
Account::~Account()
{
    for (Characters::iterator i = mCharacters.begin(),
         i_end = mCharacters.end(); i != i_end; ++i)
    {
        delete *i;
    }
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
void Account::addCharacter(Character *character)
{
    mCharacters.push_back(character);
}

void Account::delCharacter(int i)
{
    mCharacters.erase(mCharacters.begin() + i);
}

void Account::setID(int id)
{
    assert(mID < 0);
    mID = id;
}

void Account::setRegistrationDate(time_t time)
{
    mRegistrationDate = time;
}

void Account::setLastLogin(time_t time)
{
    mLastLogin = time;
}
