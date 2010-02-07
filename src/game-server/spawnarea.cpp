/*
 *  The Mana Server
 *  Copyright (C) 2006-2010  The Mana World Development Team
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

#include "game-server/spawnarea.hpp"

#include "game-server/mapcomposite.hpp"
#include "game-server/monster.hpp"
#include "game-server/state.hpp"
#include "utils/logger.h"

struct SpawnAreaEventDispatch : EventDispatch
{
    SpawnAreaEventDispatch()
    {
        typedef EventListenerFactory< SpawnArea, &SpawnArea::mSpawnedListener >
            Factory;
        removed = &Factory::create< Thing, &SpawnArea::decrease >::function;
    }
};

static SpawnAreaEventDispatch spawnAreaEventDispatch;

SpawnArea::SpawnArea(MapComposite *map,
                     MonsterClass *specy,
                     const Rectangle &zone,
                     int maxBeings,
                     int spawnRate):
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

void SpawnArea::update()
{
    if (mNextSpawn > 0)
        mNextSpawn--;

    if (mNextSpawn == 0 && mNumBeings < mMaxBeings && mSpawnRate > 0)
    {
        MapComposite *map = getMap();
        const Map *realMap = map->getMap();

        // Reset the spawn area to the whole map in case of dimensionless zone
        if (mZone.w == 0 || mZone.h == 0)
        {
            mZone.x = 0;
            mZone.y = 0;
            mZone.w = realMap->getWidth() * realMap->getTileWidth();
            mZone.h = realMap->getHeight() * realMap->getTileHeight();
        }

        // Find a free spawn location. Give up after 10 tries
        int c = 10;
        Point position;
        const int x = mZone.x;
        const int y = mZone.y;
        const int width = mZone.w;
        const int height = mZone.h;

        Being *being = new Monster(mSpecy);

        if (being->getModifiedAttribute(BASE_ATTR_HP) <= 0)
        {
            //LOG_WARN("Refusing to spawn dead monster " << mSpecy->getType());
            delete being;
            being = 0;
        }

        if (being) {
            do {
                position = Point(x + rand() % width, y + rand() % height);
                c--;
            } while (!realMap->getWalk(position.x / realMap->getTileWidth(),
                                       position.y / realMap->getTileHeight(),
                                       being->getWalkMask()) && c);

            if (c) {
                being->addListener(&mSpawnedListener);
                being->setMap(map);
                being->setPosition(position);
                being->clearDestination();
                GameState::enqueueInsert(being);

                mNumBeings++;
            }
            else {
                LOG_WARN("Unable to find a free spawn location for monster "
                         << mSpecy->getType() << " on map " << map->getName()
                         << " (" << x << ',' << y << ','
                         << width << ',' << height << ')');
                delete being;
            }
        }

        // Predictable respawn intervals (can be randomized later)
        mNextSpawn = (10 * 60) / mSpawnRate;
    }
}

void SpawnArea::decrease(Thing *t)
{
    --mNumBeings;
    t->removeListener(&mSpawnedListener);
}
