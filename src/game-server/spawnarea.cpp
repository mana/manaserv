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

#include "game-server/spawnarea.hpp"

#include "game-server/mapcomposite.hpp"
#include "game-server/monster.hpp"
#include "game-server/state.hpp"
#include "utils/logger.h"

struct SpawnAreaEventDispatch: EventDispatch
{
    SpawnAreaEventDispatch()
    {
        typedef EventListenerFactory< SpawnArea, &SpawnArea::mSpawnedListener > Factory;
        removed = &Factory::create< Thing, &SpawnArea::decrease >::function;
    }
};

static SpawnAreaEventDispatch spawnAreaEventDispatch;

SpawnArea::SpawnArea(MapComposite *map, MonsterClass *specy, const Rectangle &zone, int maxBeings, int spawnRate):
    Thing(OBJECT_OTHER, map),
    mSpecy(specy),
    mSpawnedListener(&spawnAreaEventDispatch),
    mZone(zone),
    mMaxBeings(maxBeings),
    mSpawnRate(spawnRate),
    mNumBeings(0),
    mNextSpawn(0)
{
}

void
SpawnArea::update()
{
    if (mNextSpawn > 0)
    {
        mNextSpawn--;

        if (mNextSpawn == 0)
        {
            // Find a free spawn location. Give up after 10 tries
            int c = 10;
            Point position;
            MapComposite *map = getMap();
            Map *realMap = map->getMap();
            int x = mZone.x;
            int y = mZone.y;
            int width = mZone.w;
            int height = mZone.h;

            // Reset the spawn area to the whole map in case of dimensionless zone
            if (width == 0 || height == 0)
            {
                x = 0;
                y = 0;
                width = realMap->getWidth() * 32;
                height = realMap->getHeight() * 32;
            }

            Being *being = new Monster(mSpecy);

            do
            {
                position = Point(x + rand() % width, y + rand() % height);
                c--;
            } while (!realMap->getWalk(position.x / 32, position.y / 32, being->getWalkMask()) && c);

            if (c)
            {
                being->addListener(&mSpawnedListener);

                being->setMap(map);
                being->setPosition(position);
                being->clearDestination();
                GameState::enqueueInsert(being);

                mNumBeings++;
            }
            else
            {
                LOG_WARN("Unable to find a free spawn location for monster "
                         << mSpecy->getType() << " on map " << map->getName()
                         << " (" << x << ',' << y << ','
                         << width << ',' << height << ')');
                delete being;
            }
        }
    }

    if (mNextSpawn == 0 && mNumBeings < mMaxBeings && mSpawnRate > 0)
    {
        // Predictable respawn intervals (can be randomized later)
        mNextSpawn = (10 * 60) / mSpawnRate;
    }
}

void SpawnArea::decrease(Thing *t)
{
    --mNumBeings;
    t->removeListener(&mSpawnedListener);
}
