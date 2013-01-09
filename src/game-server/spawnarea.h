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

#ifndef SPAWNAREA_H
#define SPAWNAREA_H

#include "game-server/entity.h"
#include "utils/point.h"

class Being;
class MonsterClass;

/**
 * A spawn area, where monsters spawn. The area is a rectangular field and will
 * spawn a certain number of a given monster type.
 */
class SpawnArea : public Entity
{
    public:
        SpawnArea(MapComposite *, MonsterClass *, const Rectangle &zone,
            int maxBeings, int spawnRate);

        void update();

        /**
         * Keeps track of the number of spawned being.
         */
        void decrease(Entity *);

    private:
        MonsterClass *mSpecy; /**< Specy of monster that spawns in this area. */
        Rectangle mZone;
        int mMaxBeings;    /**< Maximum population of this area. */
        int mSpawnRate;    /**< Number of beings spawning per minute. */
        int mNumBeings;    /**< Current population of this area. */
        int mNextSpawn;    /**< The time until next being spawn. */

        friend struct SpawnAreaEventDispatch;
};

#endif
