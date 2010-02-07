/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>

#include "account-server/account.hpp"

Account::~Account()
{
    for (Characters::iterator i = mCharacters.begin(),
         i_end = mCharacters.end(); i != i_end; ++i)
    {
        delete *i;
    }
}

void Account::setCharacters(const Characters& characters)
{
    mCharacters = characters;
}

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
