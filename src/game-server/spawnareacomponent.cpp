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

#include "game-server/spawnareacomponent.h"

#include "game-server/mapcomposite.h"
#include "game-server/monster.h"
#include "game-server/state.h"
#include "utils/logger.h"

const ComponentType SpawnAreaComponent::type;

SpawnAreaComponent::SpawnAreaComponent(MonsterClass *specy,
                                       const Rectangle &zone,
                                       int maxBeings,
                                       int spawnRate):
    mSpecy(specy),
    mZone(zone),
    mMaxBeings(maxBeings),
    mSpawnRate(spawnRate),
    mNumBeings(0),
    mNextSpawn(0)
{
}

void SpawnAreaComponent::update(Entity &entity)
{
    if (mNextSpawn > 0)
        mNextSpawn--;

    if (mNextSpawn == 0 && mNumBeings < mMaxBeings && mSpawnRate > 0)
    {
        MapComposite *map = entity.getMap();
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
        int triesLeft = 10;
        Point position;
        const int x = mZone.x;
        const int y = mZone.y;
        const int width = mZone.w;
        const int height = mZone.h;

        Actor *being = new Actor(OBJECT_MONSTER);
        auto *beingComponent = new BeingComponent(*being);
        being->addComponent(beingComponent);
        being->addComponent(new MonsterComponent(*being, mSpecy));

        if (beingComponent->getModifiedAttribute(ATTR_MAX_HP) <= 0)
        {
            LOG_WARN("Refusing to spawn dead monster " << mSpecy->getId());
            delete being;
            being = 0;
        }

        if (being)
        {
            do
            {
                position = Point(x + rand() % width, y + rand() % height);
                triesLeft--;
            }
            while (!realMap->getWalk(position.x / realMap->getTileWidth(),
                                     position.y / realMap->getTileHeight(),
                                     being->getWalkMask())
                   && triesLeft);

            if (triesLeft)
            {
                being->signal_removed.connect(
                            sigc::mem_fun(this, &SpawnAreaComponent::decrease));

                being->setMap(map);
                being->setPosition(position);
                beingComponent->clearDestination(*being);
                GameState::enqueueInsert(being);

                mNumBeings++;
            }
            else
            {
                LOG_WARN("Unable to find a free spawn location for monster "
                         << mSpecy->getId() << " on map " << map->getName()
                         << " (" << x << ',' << y << ','
                         << width << ',' << height << ')');
                delete being;
            }
        }

        // Predictable respawn intervals (can be randomized later)
        mNextSpawn = (10 * 60) / mSpawnRate;
    }
}

void SpawnAreaComponent::decrease(Entity *)
{
    --mNumBeings;
}
