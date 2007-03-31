/*
 *  The Mana World Server
 *  Copyright 2006 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#include "spawnarea.hpp"

#include "game-server/monster.hpp"
#include "game-server/state.hpp"

#include "utils/logger.h"

/*
 * TODO: Allow specifying being type and use it.
 */

SpawnArea::SpawnArea(int mapId, const Rectangle &zone):
    Thing(OBJECT_OTHER),
    mZone(zone),
    mMaxBeings(10),
    mBeingType(1),
    mSpawnRate(10),
    mNumBeings(0),
    mNextSpawn(0)
{
    setMapId(mapId);
}

void
SpawnArea::update()
{
    if (mNextSpawn > 0)
    {
        mNextSpawn--;

        if (mNextSpawn == 0)
        {
            Being *being = new Monster();
            being->addDeathListener(this);

            // some bogus stats for testing
            being->setSpeed(150);
            being->setSize(8);
            being->setAttribute(BASE_ATTR_VITALITY, 10);
            being->fillHitpoints();

            being->setMapId(1);
            being->setPosition(Point(mZone.x + rand() % mZone.w,
                        mZone.y + rand() % mZone.h));
            gameState->insert(being);

            mNumBeings++;
        }
    }

    if (mNextSpawn == 0 && mNumBeings < mMaxBeings && mSpawnRate > 0)
    {
        // Predictable respawn intervals (can be randomized later)
        mNextSpawn = (10 * 60) / mSpawnRate;
    }
}

void
SpawnArea::died(Being *being)
{
    mNumBeings--;
}
