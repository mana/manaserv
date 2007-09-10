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

#ifndef _TMWSERV_SPAWNAREA
#define _TMWSERV_SPAWNAREA

#include "point.h"
#include "game-server/eventlistener.hpp"
#include "game-server/thing.hpp"

class Being;
class MonsterClass;

/**
 * A spawn area, where monsters spawn. The area is a rectangular field and will
 * spawn a certain number of a given monster type.
 */
class SpawnArea : public Thing
{
    public:
        SpawnArea(MapComposite *, MonsterClass *, Rectangle const &zone,
            int maxBeings, int spawnRate);

        void update();

        /**
         * Keeps track of the number of spawned being.
         */
        void decrease(Thing *);

    private:
        MonsterClass *mSpecy; /**< Specy of monster that spawns in this area. */
        EventListener mSpawnedListener; /**< Tracking of spawned monsters. */
        Rectangle mZone;
        int mMaxBeings;    /**< Maximum population of this area. */
        int mSpawnRate;    /**< Number of beings spawning per minute. */
        int mNumBeings;    /**< Current population of this area. */
        int mNextSpawn;    /**< The time until next being spawn. */

        friend struct SpawnAreaEventDispatch;
};

#endif
