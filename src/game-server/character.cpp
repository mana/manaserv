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

#include "game-server/character.hpp"

#include "defines.h"
#include "game-server/buysell.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/trade.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "serialize/characterdata.hpp"

Character::Character(MessageIn & msg):
    Being(OBJECT_CHARACTER, 65535),
    mClient(NULL), mTransactionHandler(NULL), mDatabaseID(-1),
    mGender(0), mHairStyle(0), mHairColor(0), mLevel(0),
    mTransaction(TRANS_NONE), mAttributesChanged(true)
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

int Character::getMapId() const
{
    return getMap()->getID();
}

void Character::setMapId(int id)
{
    setMap(MapManager::getMap(id));
}

void Character::cancelTransaction()
{
    TransactionType t = mTransaction;
    mTransaction = TRANS_NONE;
    switch (t)
    {
        case TRANS_TRADE:
            static_cast< Trade * >(mTransactionHandler)->cancel(this);
            break;
        case TRANS_BUYSELL:
            static_cast< BuySell * >(mTransactionHandler)->cancel();
            break;
        case TRANS_NONE:
            return;
    }
}

Trade *Character::getTrading() const
{
    return mTransaction == TRANS_TRADE
        ? static_cast< Trade * >(mTransactionHandler) : NULL;
}

BuySell *Character::getBuySell() const
{
    return mTransaction == TRANS_BUYSELL
        ? static_cast< BuySell * >(mTransactionHandler) : NULL;
}

void Character::setTrading(Trade *t)
{
    cancelTransaction();
    mTransactionHandler = t;
    mTransaction = TRANS_TRADE;
}

void Character::setBuySell(BuySell *t)
{
    cancelTransaction();
    mTransactionHandler = t;
    mTransaction = TRANS_BUYSELL;
}

