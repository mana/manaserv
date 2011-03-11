/*
 *  The Mana Server
 *  Copyright (C) 2004-2011  The Mana World Development Team
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

#ifndef MAP_H
#define MAP_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "utils/point.h"

typedef std::list<Point> Path;
typedef Path::iterator PathIterator;

enum BlockType
{
    BLOCKTYPE_NONE = -1,
    BLOCKTYPE_WALL,
    BLOCKTYPE_CHARACTER,
    BLOCKTYPE_MONSTER,
    NB_BLOCKTYPES
};

/**
 * A meta tile stores additional information about a location on a tile map.
 * This is information that doesn't need to be repeated for each tile in each
 * layer of the map.
 */
class MetaTile
{
    public:
        MetaTile()
            : blockmask(0)
        {
            for (unsigned i = 0; i < NB_BLOCKTYPES; ++i)
                occupation[i] = 0;
        }

        unsigned occupation[NB_BLOCKTYPES];
        char blockmask;          /**< walkability bitfield */
};

/**
 * A tile map.
 */
class Map
{
    public:
        /**
         * Constructor that takes initial map size as parameters.
         */
        Map(int width, int height,
            int tileWidth, int tileHeight);

        /**
         * Sets the size of the map. This will destroy any existing map data.
         */
        void setSize(int width, int height);

        /**
         * Marks a tile as occupied
         */
        void blockTile(int x, int y, BlockType type);

        /**
         * Marks a tile as unoccupied
         */
        void freeTile(int x, int y, BlockType type);

        /**
         * Gets walkability for a tile with a blocking bitmask
         */
        bool getWalk(int x, int y, char walkmask = BLOCKMASK_WALL) const;

        /**
         * Tells if a tile location is within the map range.
         */
        bool contains(int x, int y) const
        { return x >= 0 && y >= 0 && x < mWidth && y < mHeight; }

        /**
         * Returns the width of this map.
         */
        int getWidth() const
        { return mWidth; }

        /**
         * Returns the height of this map.
         */
        int getHeight() const
        { return mHeight; }

        /**
         * Returns the tile width of this map.
         */
        int getTileWidth() const
        { return mTileWidth; }

        /**
         * Returns the tile height used by this map.
         */
        int getTileHeight() const
        { return mTileHeight; }

        /**
         * Returns a general map property defined in the map file
         */
        const std::string &getProperty(const std::string &key) const;

        /**
        * Sets a map property
        */
        void setProperty(const std::string& key, const std::string& val)
        { mProperties[key] = val; }

        /**
         * Find a path from one location to the next.
         */
        Path findPath(int startX, int startY,
                      int destX, int destY,
                      unsigned char walkmask,
                      int maxCost = 20) const;

    private:
        /**
         * Blockmasks for different entities
         */
        static const unsigned char BLOCKMASK_WALL = 0x80;     // = bin 1000 0000
        static const unsigned char BLOCKMASK_CHARACTER = 0x01;// = bin 0000 0001
        static const unsigned char BLOCKMASK_MONSTER = 0x02;  // = bin 0000 0010

        // map properties
        int mWidth, mHeight;
        int mTileWidth, mTileHeight;
        std::map<std::string, std::string> mProperties;

        std::vector<MetaTile> mMetaTiles;
};

#endif
