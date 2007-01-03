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
#include "game-server/mapmanager.hpp"
#include "game-server/object.hpp"

void MovingObject::move()
{
    mOld = getPosition();
    if (mActionTime > 100)
    {
        // current move has not yet ended
        mActionTime -= 100;
        return;
    }

    int tileSX = mOld.x / 32, tileSY = mOld.y / 32;
    int tileDX = mDst.x / 32, tileDY = mDst.y / 32;
    if (tileSX == tileDX && tileSY == tileDY)
    {
        // moving while staying on the same tile is free
        setPosition(mDst);
        mActionTime = 0;
        return;
    }

    Map *map = mapManager->getMap(getMapId());
    // TODO: cache pathfinding results
    std::list<PATH_NODE> path = map->findPath(tileSX, tileSY, tileDX, tileDY);
    if (path.empty())
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
        PATH_NODE next = path.front();
        path.pop_front();
        mActionTime += (prev.x != next.x && prev.y != next.y)
                       ? mSpeed * 362 / 256 : mSpeed;
        if (path.empty())
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
