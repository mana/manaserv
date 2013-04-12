/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
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

#include "account-server/character.h"

#include "account-server/account.h"

CharacterData::CharacterData(const std::string &name, int id):
    mName(name),
    mDatabaseID(id),
    mCharacterSlot(0),
    mAccountID(-1),
    mAccount(NULL),
    mMapId(0),
    mGender(0),
    mHairStyle(0),
    mHairColor(0),
    mLevel(0),
    mCharacterPoints(0),
    mCorrectionPoints(0),
    mAccountLevel(0)
{
}

void CharacterData::setAccount(Account *acc)
{
    mAccount = acc;
    mAccountID = acc->getID();
    mAccountLevel = acc->getLevel();
}

void CharacterData::giveSpecial(int id, int currentMana)
{
    if (mSpecials.find(id) == mSpecials.end())
    {
        mSpecials[id] = SpecialValue(currentMana);
    }
}
