/*
 *  The Mana World
 *  Copyright 2004 The Mana World Development Team
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

#ifndef _TMW_MAP_H
#define _TMW_MAP_H

#include <list>
#include <map>
#include <string>


struct PATH_NODE {
    PATH_NODE(unsigned short u, unsigned short v)
        : x(u), y(v)
    {}

    unsigned short x, y;
};

/**
 * A meta tile stores additional information about a location on a tile map.
 * This is information that doesn't need to be repeated for each tile in each
 * layer of the map.
 */
class MetaTile
{
    public:
        /**
         * Constructor.
         */
        MetaTile();

        // Pathfinding members
        int Fcost;              /**< Estimation of total path cost */
        int Gcost;              /**< Cost from start to this location */
        int Hcost;              /**< Estimated cost to goal */
        int whichList;          /**< No list, open list or closed list */
        int parentX;            /**< X coordinate of parent tile */
        int parentY;            /**< Y coordinate of parent tile */
        bool permWalkable;      /**< Can beings normally walk on this tile */
        bool tempWalkable;      /**< Can beings walk on this tile this tick? */
};

/**
 * A location on a tile map. Used for pathfinding, open list.
 */
class Location
{
    public:
        /**
         * Constructor.
         */
        Location(int x, int y, MetaTile *tile);

        /**
         * Comparison operator.
         */
        bool operator< (const Location &loc) const;

        int x, y;
        MetaTile *tile;
};

/**
 * A tile map.
 */
class Map
{
    public:
        /**
         * Constructor.
         */
        Map();

        /**
         * Constructor that takes initial map size as parameters.
         */
        Map(int width, int height);

        /**
         * Destructor.
         */
        ~Map();

        /**
         * Sets the size of the map. This will destroy any existing map data.
         */
        void
        setSize(int width, int height);

        /**
         * Get tile reference.
         */
        MetaTile*
        getMetaTile(int x, int y);

        /**
         * Set permanent walkability flag for a tile
         */
        void
        setPermWalk(int x, int y, bool walkable);

        /**
         * Set temporary walkability flag for a tile
         */
        void
        setTempWalk(int x, int y, bool walkable);

        /**
         * Resets the temporary walkable status of all tiles to the permanent
         * walkable status.
         */
        void resetTempWalk();

        /**
         * Tell if a tile is walkable or not, includes checking beings.
         */
        bool
        getWalk(int x, int y);

        /**
         * Tell if a tile collides, not including a check on beings.
         */
        bool
        tileCollides(int x, int y);

        /**
         * Returns the width of this map.
         */
        int getWidth() const
        { return width; }

        /**
         * Returns the height of this map.
         */
        int getHeight() const
        { return height; }

        /**
         * Returns the tile width of this map.
         */
        int getTileWidth() const
        { return tileWidth; }

        /**
         * Returns the tile height used by this map.
         */
        int getTileHeight() const
        { return tileHeight; }

        /**
         * Find a path from one location to the next.
         */
        std::list<PATH_NODE>
        findPath(int startX, int startY, int destX, int destY, int maxCost = 20);

    private:
        int width, height;
        int tileWidth, tileHeight;
        MetaTile *metaTiles;

        // Pathfinding members
        int onClosedList, onOpenList;
};

#endif
