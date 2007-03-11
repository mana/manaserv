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

Player::Player(std::string const &name, int id)
          : Being(OBJECT_PLAYER, 65535),
            PlayerData(name, id),
            mClient(NULL)
{
    mStats.base.resize(NB_STATS_PLAYER, 1); //TODO: fill with the real values
    mStats.absoluteModificator.resize(NB_STATS_PLAYER, 0);
    mStats.percentModificators.resize(NB_STATS_PLAYER);

    // some bogus values for testing purpose
    mStats.base.at(STRENGTH) = 10;
    mStats.base.at(SKILL_WEAPON_UNARMED) = 5;

    calculateBaseStats();

    mHitpoints = getRealStat(STAT_HP_MAXIMUM);
    mSize = 16;
}

/**
 * Update the internal status.
 */
void Player::update()
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

void Player::calculateBaseStats()
{
    mStats.base.at(STAT_HP_MAXIMUM)
        = getRealStat(VITALITY);

    mStats.base.at(STAT_PHYSICAL_ATTACK_MINIMUM)
        = getRealStat(STRENGTH) /* + weapon damage fluctuation*/;

    // TODO: get the skill that is skill required for weapon
    mStats.base.at(STAT_PHYSICAL_ATTACK_FLUCTUATION)
        = getRealStat(SKILL_WEAPON_UNARMED) /* + weapon damage fluctuation*/;

    mStats.base.at(STAT_PHYSICAL_DEFENCE)
        = 42 /* + sum of equipment pieces */;
}
