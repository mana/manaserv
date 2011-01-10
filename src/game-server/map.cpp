/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
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

#include <algorithm>
#include <queue>
#include <cassert>
#include <cstring>

#include "game-server/map.h"

#include "defines.h"

// Basic cost for moving from one tile to another.
// Used in findPath() function when computing the A* path algorithm.
static int const basicCost = 100;

MetaTile::MetaTile():
    whichList(0),
    blockmask(0)
{ }

Location::Location(int x, int y, MetaTile *tile):
    x(x), y(y), tile(tile)
{ }

bool Location::operator< (const Location &loc) const
{
   return tile->Fcost > loc.tile->Fcost;
}

Map::Map(int width, int height, int twidth, int theight):
    mWidth(width), mHeight(height),
    mTileWidth(twidth), mTileHeight(theight),
    onClosedList(1), onOpenList(2)
{
    mMetaTiles = new MetaTile[mWidth * mHeight];
    for (int i=0; i < NB_BLOCKTYPES; i++)
    {
        mOccupation[i] = new int[mWidth * mHeight];
        memset(mOccupation[i], 0, mWidth * mHeight * sizeof(int));
    }
}

Map::~Map()
{
    delete[] mMetaTiles;
    for (int i=0; i < NB_BLOCKTYPES; i++)
    {
        delete[] mOccupation[i];
    }
}

void Map::setSize(int width, int height)
{
    this->mWidth = width;
    this->mHeight = height;

    delete[] mMetaTiles;
    mMetaTiles = new MetaTile[mWidth * mHeight];

    for (int i=0; i < NB_BLOCKTYPES; i++)
    {
        delete[] mOccupation[i];
        mOccupation[i] = new int[mWidth * mHeight];
    }
}

const std::string &Map::getProperty(const std::string &key) const
{
    static std::string empty;
    std::map<std::string, std::string>::const_iterator i;
    i = mProperties.find(key);
    if (i == mProperties.end())
        return empty;
    return i->second;
}

void Map::blockTile(int x, int y, BlockType type)
{
    if (type == BLOCKTYPE_NONE || x < 0 || y < 0 || x >= mWidth || y >= mHeight)
    {
        return;
    }

    int tileNum = x + y * mWidth;

    if (++mOccupation[type][tileNum])
    {
        switch (type)
        {
            case BLOCKTYPE_WALL:
                mMetaTiles[tileNum].blockmask |= BLOCKMASK_WALL;
                break;
            case BLOCKTYPE_CHARACTER:
                mMetaTiles[tileNum].blockmask |= BLOCKMASK_CHARACTER;
                break;
            case BLOCKTYPE_MONSTER:
                mMetaTiles[tileNum].blockmask |= BLOCKMASK_MONSTER;
                break;
            default:
                // shut up!
                break;
        }
    }
}

void Map::freeTile(int x, int y, BlockType type)
{
    if (type == BLOCKTYPE_NONE || x < 0 || y < 0 || x >= mWidth || y >= mHeight)
    {
        return;
    }

    int tileNum = x + y * mWidth;

    if (!(--mOccupation[type][tileNum]))
    {
        switch (type)
        {
            case BLOCKTYPE_WALL:
                mMetaTiles[tileNum].blockmask &= (BLOCKMASK_WALL xor 0xff);
                break;
            case BLOCKTYPE_CHARACTER:
                mMetaTiles[tileNum].blockmask &= (BLOCKMASK_CHARACTER xor 0xff);
                break;
            case BLOCKTYPE_MONSTER:
                mMetaTiles[tileNum].blockmask &= (BLOCKMASK_MONSTER xor 0xff);
                break;
            default:
                // nothing
                break;
        }
    }
}

bool Map::getWalk(int x, int y, char walkmask) const
{
    // You can't walk outside of the map
    if (!contains(x, y))
    {
        return false;
    }

    // Check if the tile is walkable
    return !(mMetaTiles[x + y * mWidth].blockmask & walkmask);
}

MetaTile *Map::getMetaTile(int x, int y)
{
    return &mMetaTiles[x + y * mWidth];
}

bool Map::contains(int x, int y) const
{
    return x >= 0 && y >= 0 && x < mWidth && y < mHeight;
}

Path Map::findSimplePath(int startX, int startY,
                                         int destX, int destY,
                                         unsigned char walkmask)
{
    // Path to be built up (empty by default)
    Path path;
    int positionX = startX, positionY = startY;
    int directionX, directionY;
    // Checks our path up to 1 tiles, if a blocking tile is found
    // We go to the last good tile, and break out of the loop
    while(true)
    {
        directionX = destX - positionX;
        directionY = destY - positionY;

        if (directionX > 0)
            directionX = 1;
        else if(directionX < 0)
            directionX = -1;

        if (directionY > 0)
            directionY = 1;
        else if(directionY < 0)
            directionY = -1;

        positionX += directionX;
        positionY += directionY;

        if (getWalk(positionX, positionY, walkmask))
        {
            path.push_back(Point(positionX, positionY));

            if ((positionX == destX) && (positionY == destY))
            {
                return path;
            }
        }
        else
        {
            return path;
        }
    }
}

Path Map::findPath(int startX, int startY,
                                   int destX, int destY,
                                   unsigned char walkmask, int maxCost)
{
    // Path to be built up (empty by default)
    Path path;

    // Declare open list, a list with open tiles sorted on F cost
    std::priority_queue<Location> openList;

    // Return when destination not walkable
    if (!getWalk(destX, destY, walkmask)) return path;

    // Reset starting tile's G cost to 0
    MetaTile *startTile = getMetaTile(startX, startY);
    startTile->Gcost = 0;

    // Add the start point to the open list
    openList.push(Location(startX, startY, startTile));

    bool foundPath = false;

    // Keep trying new open tiles until no more tiles to try or target found
    while (!openList.empty() && !foundPath)
    {
        // Take the location with the lowest F cost from the open list, and
        // add it to the closed list.
        Location curr = openList.top();
        openList.pop();

        // If the tile is already on the closed list, this means it has already
        // been processed with a shorter path to the start point (lower G cost)
        if (curr.tile->whichList == onClosedList)
            continue;

        // Put the current tile on the closed list
        curr.tile->whichList = onClosedList;

        // Check the adjacent tiles
        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dx = -1; dx <= 1; dx++)
            {
                // Calculate location of tile to check
                int x = curr.x + dx;
                int y = curr.y + dy;

                // Skip if if we're checking the same tile we're leaving from,
                // or if the new location falls outside of the map boundaries
                if ((dx == 0 && dy == 0) || !contains(x, y))
                    continue;

                MetaTile *newTile = getMetaTile(x, y);

                // Skip if the tile is on the closed list or is not walkable
                if (newTile->whichList == onClosedList || newTile->blockmask & walkmask)
                    continue;

                // When taking a diagonal step, verify that we can skip the
                // corner.
                if (dx != 0 && dy != 0)
                {
                    MetaTile *t1 = getMetaTile(curr.x, curr.y + dy);
                    MetaTile *t2 = getMetaTile(curr.x + dx, curr.y);

                    if ((t1->blockmask | t2->blockmask) & walkmask)
                        continue;
                }

                // Calculate G cost for this route, ~sqrt(2) for moving diagonal
                int Gcost = curr.tile->Gcost +
                    (dx == 0 || dy == 0 ? basicCost : basicCost * 362 / 256);

                /* Demote an arbitrary direction to speed pathfinding by
                   adding a defect (TODO: change depending on the desired
                   visual effect, e.g. a cross-product defect toward
                   destination).
                   Important: as long as the total defect along any path is
                   less than the basicCost, the pathfinder will still find one
                   of the shortest paths! */
                if (dx == 0 || dy == 0)
                {
                    // Demote horizontal and vertical directions, so that two
                    // consecutive directions cannot have the same Fcost.
                    ++Gcost;
                }

                // Skip if Gcost becomes too much
                // Warning: probably not entirely accurate
                if (Gcost > maxCost * basicCost)
                    continue;

                if (newTile->whichList != onOpenList)
                {
                    // Found a new tile (not on open nor on closed list)

                    /* Update Hcost of the new tile. The pathfinder does not
                       work reliably if the heuristic cost is higher than the
                       real cost. In particular, using Manhattan distance is
                       forbidden here. */
                    int dx = std::abs(x - destX), dy = std::abs(y - destY);
                    newTile->Hcost = std::abs(dx - dy) * basicCost +
                        std::min(dx, dy) * (basicCost * 362 / 256);

                    // Set the current tile as the parent of the new tile
                    newTile->parentX = curr.x;
                    newTile->parentY = curr.y;

                    // Update Gcost and Fcost of new tile
                    newTile->Gcost = Gcost;
                    newTile->Fcost = newTile->Gcost + newTile->Hcost;

                    if (x != destX || y != destY) {
                        // Add this tile to the open list
                        newTile->whichList = onOpenList;
                        openList.push(Location(x, y, newTile));
                    }
                    else
                    {
                        // Target location was found
                        foundPath = true;
                    }
                }
                else if (Gcost < newTile->Gcost)
                {
                    // Found a shorter route.
                    // Update Gcost and Fcost of the new tile
                    newTile->Gcost = Gcost;
                    newTile->Fcost = Gcost + newTile->Hcost;

                    // Set the current tile as the parent of the new tile
                    newTile->parentX = curr.x;
                    newTile->parentY = curr.y;

                    // Add this tile to the open list (it's already
                    // there, but this instance has a lower F score)
                    openList.push(Location(x, y, newTile));
                }
            }
        }
    }

    // Two new values to indicate whether a tile is on the open or closed list,
    // this way we don't have to clear all the values between each pathfinding.
    onClosedList += 2;
    onOpenList += 2;

    // If a path has been found, iterate backwards using the parent locations
    // to extract it.
    if (foundPath)
    {
        int pathX = destX;
        int pathY = destY;

        while (pathX != startX || pathY != startY)
        {
            // Add the new path node to the start of the path list
            path.push_front(Point(pathX, pathY));

            // Find out the next parent
            MetaTile *tile = getMetaTile(pathX, pathY);
            pathX = tile->parentX;
            pathY = tile->parentY;
        }
    }

    return path;
}
