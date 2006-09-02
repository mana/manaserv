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
 *  $Id: $
 */

#ifndef _TMW_SERVER_MAPCOMPOSITE_
#define _TMW_SERVER_MAPCOMPOSITE_

#include "object.h"

class Player;
class Map;

/**
 * Pool of MovingObject.
 */
struct ObjectBucket
{
    unsigned bitmap[256 / sizeof(unsigned)]; /**< Bitmap of free locations. */
    short free; /**< Number of empty places. */
    short next_object; /**< Next object to look at. */
    MovingObject *objects[256];

    ObjectBucket();
    int allocate();
    void deallocate(int);
};

/**
 * Combined map/entity structure.
 */
class MapComposite {

    public:
        MapComposite();
        ~MapComposite();

        /**
         * Actual map.
         */
        Map *map;

        /**
         * Objects (items, players, monsters, etc) located on the map.
         */
        Objects objects;

        /**
         * Players located on the map. Already owned by the objects field.
         */
        std::vector< Player * > players;

        /**
         * Allocates a unique ID for a moving object on this map.
         */
        void allocate(MovingObject *);

        /**
         * Deallocates an ID.
         */
        void deallocate(MovingObject *);

        /**
         * Gets an object given its ID.
         */
        MovingObject *getObjectByID(int) const;

    private:
        MapComposite(MapComposite const &);

        /**
         * Buckets of MovingObject located on the map, referenced by ID.
         */
        ObjectBucket *buckets[256];

        int last_bucket; /**< Last bucket acted upon. */
};

#endif
