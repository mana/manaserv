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

#include "defines.h"
#include "game-server/character.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "serialize/characterdata.hpp"

Character::Character(MessageIn & msg):
    Being(OBJECT_CHARACTER, 65535),
    mClient(NULL),
    mAttributesChanged(true),
    mDatabaseID(-1), mName(), mGender(0), mHairStyle(0), mHairColor(0),
    mLevel(0), mMoney(0)
{
    // prepare attributes vector
    mAttributes.resize(NB_ATTRIBUTES_CHAR, 1);
    mOldAttributes.resize(NB_ATTRIBUTES_CHAR, 0);
    // get character data
    mDatabaseID = msg.readLong();
    mName = msg.readString();
    deserializeCharacterData(*this, msg);
    // give the player 10 weapon skill for testing purpose
    setAttribute(CHAR_SKILL_WEAPON_UNARMED, 10);

    setSize(16);
}
/**
 * Update the internal status.
 */
void
Character::update()
{
    // attacking
    if (mAction == ATTACK)
    {
        // plausibility check of attack command
        if (mActionTime <= 0)
        {
            // request perform attack
            mActionTime = 1000;
            mAction = STAND;
            raiseUpdateFlags(UPDATEFLAG_ATTACK);
        }
    }
}

void Character::calculateDerivedAttributes()
{
    Being::calculateDerivedAttributes();
    /*
     * Do any player character specific attribute calculation here
     */

    mAttributesChanged = true;
}

WeaponStats
Character::getWeaponStats()
{
    WeaponStats weaponStats;

    /*
     * TODO: get all this stuff from the currently equipped weapon
     */
    weaponStats.piercing = 1;
    weaponStats.element = ELEMENT_NEUTRAL;
    weaponStats.skill = CHAR_SKILL_WEAPON_UNARMED;

    return weaponStats;
}

void
Character::writeAttributeUpdateMessage(MessageOut &msg)
{
    if (!mAttributesChanged) return;

    for (int i = 0; i<NB_ATTRIBUTES_CHAR; i++)
    {
        unsigned short attribute = getAttribute(i);
        if (attribute != mOldAttributes[i])
        {
            msg.writeShort(i);
            msg.writeShort(attribute);
            mOldAttributes[i] = attribute;
        }
    }

    mAttributesChanged = false;
}
