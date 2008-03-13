/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */

#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/movingobject.hpp"


void MovingObject::setPosition(const Point &p)
{
    //update blockmap
    if (getMap())
    {
        Point oldP = getPosition();
        if ((oldP.x / 32 != p.x / 32 || oldP.y / 32 != p.y / 32))
        {
            getMap()->getMap()->freeTile(oldP.x / 32, oldP.y / 32, getBlockType());
            getMap()->getMap()->blockTile(p.x / 32, p.y / 32, getBlockType());
        }
    }

    Object::setPosition(p);
}

void MovingObject::setMap(MapComposite *map)
{
    Point p = getPosition();
    MapComposite *oldMap = getMap();
    if (oldMap)
    {
        oldMap->getMap()->freeTile(p.x / 32, p.y / 32, getBlockType());
    }
    map->getMap()->blockTile(p.x / 32, p.y / 32, getBlockType());
    Object::setMap(map);
}

void MovingObject::setDestination(Point const &dst)
{
    mDst = dst;
    raiseUpdateFlags(UPDATEFLAG_NEW_DESTINATION);
    mPath.clear();
}

void MovingObject::move()
{
    mOld = getPosition();
    if (mActionTime > 100)
    {
        // Current move has not yet ended
        mActionTime -= 100;
        return;
    }

    int tileSX = mOld.x / 32, tileSY = mOld.y / 32;
    int tileDX = mDst.x / 32, tileDY = mDst.y / 32;
    if (tileSX == tileDX && tileSY == tileDY)
    {
        // Moving while staying on the same tile is free
        setPosition(mDst);
        mActionTime = 0;
        return;
    }

    Map *map = getMap()->getMap();

    /* If no path exists, the for-loop won't be entered. Else a path for the
     * current destination has already been calculated.
     * The tiles in this path have to be checked for walkability,
     * in case there have been changes. The 'getWalk' method of the Map
     * class has been used, because that seems to be the most logical
     * place extra functionality will be added.
     */
    for (std::list<PATH_NODE>::iterator pathIterator = mPath.begin();
            pathIterator != mPath.end(); pathIterator++)
    {
        if (!map->getWalk(pathIterator->x, pathIterator->y, getWalkMask()))
        {
            mPath.clear();
            break;
        }
    }

    if (mPath.empty())
    {
        // No path exists: the walkability of cached path has changed, the
        // destination has changed, or a path was never set.
        mPath = map->findPath(tileSX, tileSY, tileDX, tileDY, getWalkMask());
    }

    if (mPath.empty())
    {
        // no path was found
        mDst = mOld;
        mActionTime = 0;
        return;
    }

    PATH_NODE prev(tileSX, tileSY);
    Point pos;
    do
    {
        PATH_NODE next = mPath.front();
        mPath.pop_front();
        mActionTime += (prev.x != next.x && prev.y != next.y)
                       ? mSpeed * 362 / 256 : mSpeed;
        if (mPath.empty())
        {
            // skip last tile center
            pos = mDst;
            break;
        }
        pos.x = next.x * 32 + 16;
        pos.y = next.y * 32 + 16;
    }
    while (mActionTime < 100);
    setPosition(pos);

    mActionTime = mActionTime > 100 ? mActionTime - 100 : 0;
}
