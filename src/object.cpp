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

#include "object.h"

#include "map.h"
#include "mapmanager.h"

void MovingObject::move()
{
    mOld = getPosition();
    if (mActionTime > 100)
    {
        // current move has not yet ended
        mActionTime -= 100;
        return;
    }

    Map *map = MapManager::instance().getMap(getMapId());
    std::list<PATH_NODE> path;
    int tileSX = mOld.x / 32, tileSY = mOld.y / 32;
    int tileDX = mDst.x / 32, tileDY = mDst.y / 32;
    if (tileSX != tileDX || tileSY != tileDY)
    {
        path = map->findPath(tileSX, tileSY, tileDX, tileDY);
        if (path.empty()) {
            // no path was found
            mDst = mOld;
            return;
        }
        // last tile center is skipped
        path.pop_back();
    }

    Point pos;
    do
    {
        mActionTime += mSpeed;
        if (path.empty())
        {
            // skip last tile center
            setPosition(mDst);
            return;
        }
        pos.x = path.front().x * 32 + 16;
        pos.y = path.front().y * 32 + 16;
        path.pop_front();
    }
    while (mActionTime < 100);
    setPosition(pos);
}
