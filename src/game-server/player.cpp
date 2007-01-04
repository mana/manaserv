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
#include "game-server/player.hpp"

/**
 * Update the internal status.
 */
void Player::update()
{
    // computed stats.
    setStat(STAT_HEAT, 20 + (20 * getRawStat(STAT_VITALITY)));
    setStat(STAT_ATTACK, 10 + getRawStat(STAT_STRENGTH));
    setStat(STAT_DEFENCE, 10 + getRawStat(STAT_STRENGTH));
    setStat(STAT_MAGIC, 10 + getRawStat(STAT_INTELLIGENCE));
    setStat(STAT_ACCURACY, 50 + getRawStat(STAT_DEXTERITY));
    setStat(STAT_SPEED, getRawStat(STAT_DEXTERITY));

    // Update persistent data.
    setPos(getPosition());
    setMap(getMapId());

    // attacking
    if (mIsAttacking)
    {
        // plausibility check of attack command
        if (mActionTime <= 0)
        {
            // request perform attack
            mActionTime = 1000;
            mIsAttacking = false;
            raiseUpdateFlags(ATTACK);
        }
    }
}

/*
bool Player::insertItem(int itemId, int amount)
{
    return inventory.insertItem(itemId, amount);
}

bool Player::removeItem(int itemId, int amount)
{
    return inventory.removeItemById(itemId, amount);
}

bool Player::hasItem(int itemId)
{
    return inventory.hasItem(itemId);
}

bool Player::equip(int slot)
{
    return false; // TODO
}

bool Player::unequip(int slot)
{
    return false; // TODO
}
*/
