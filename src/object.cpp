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

#include <cassert>
#include <cmath>

#include "object.h"

#include "map.h"
#include "mapmanager.h"

void Object::setID(int id)
{
    assert(mID < 0);
    mID = id;
}

void MovingObject::move()
{
    unsigned mSrcX = getX(), mSrcY = getY();
    if (mSrcX == mDstX && mSrcY == mDstY) {
        mNewX = mDstX;
        mNewY = mDstY;
        return;
    }

    Map *map = MapManager::instance().getMap(getMapId());
    int tileW = map->getTileWidth(), tileH = map->getTileHeight();
    int tileD = (int) std::sqrt((double)(tileW * tileW + tileH * tileH));
    int tileSX = mSrcX / tileW, fracSX = mSrcX % tileW - tileW / 2;
    int tileSY = mSrcY / tileH, fracSY = mSrcY % tileH - tileH / 2;
    int tileDX = mDstX / tileW, fracDX = mDstX % tileW - tileW / 2;
    int tileDY = mDstY / tileH, fracDY = mDstY % tileH - tileH / 2;

    std::list<PATH_NODE> path;
    if (tileSX != tileDX || tileSY != tileDY) {
        path = map->findPath(tileSX, tileSY, tileDX, tileDY);
        if (path.empty()) {
            mNewX = mDstX = mSrcX;
            mNewY = mDstY = mSrcY;
            return;
        }
        path.pop_back();
    }

    int tileCX = tileSX, tileCY = tileSY, fracCX = fracSX, fracCY = fracSY;
    int left = mSpeed;
    int vecX = 0, vecY = 0, cost = 0;

    for (std::list<PATH_NODE>::const_iterator it = path.begin(),
         it_end = path.end(); it != it_end; ++it)
    {
        int tileNX = it->x, tileNY = it->y;
        assert((tileNX != tileCX || tileNY != tileCY) &&
               (tileNX != tileDX || tileNY != tileDY));

        if (fracCX != 0 || fracCY != 0) {
            // not at the tile center, move toward the next tile center
            vecX = -fracCX + tileW * (tileNX - tileCX);
            vecY = -fracCY + tileH * (tileNY - tileCY);
            cost = (int) std::sqrt((double)(vecX * vecX + vecY * vecY));
        } else {
            // at a tile center and toward the next tile center
            assert(abs(tileNX - tileCX) <= 1 && abs(tileNY - tileCY) <= 1);
            if (tileNX != tileCX) {
                vecX = tileW * (tileNX - tileCX);
                if (tileNY != tileCY) {
                    vecY = tileH * (tileNY - tileCY);
                    cost = tileD;
                } else {
                    vecY = 0;
                    cost = tileW;
                }
            } else {
                assert(tileNY != tileCY);
                vecX = 0;
                vecY = tileH * (tileNY - tileCY);
                cost = tileH;
            }
        }

        if (cost > left) break;
        // enough movement left to reach the next tile center
        tileCX = tileNX;
        tileCY = tileNY;
        fracCX = 0;
        fracCY = 0;
        vecX = 0;
        vecY = 0;
        left -= cost;
    }

    if ((vecX == 0 && vecY == 0) && (fracCX != fracDX || fracCY != fracDY)) {
        // walk toward the destination
        vecX = fracDX - fracCX + tileW * (tileDX - tileCX);
        vecY = fracDY - fracCY + tileH * (tileDY - tileCY);
        cost = (int) std::sqrt((double)(vecX * vecX + vecY * vecY));
    }

    if (cost <= left) {
        // enough movement left to perform the last step
        mNewX = mDstX;
        mNewY = mDstY;
        return;
    }

    // linear interpolation along the vector
    assert(cost > 0);
    mNewX = tileCX * tileW + tileW / 2 + fracCX + vecX * left / cost;
    mNewY = tileCY * tileH + tileH / 2 + fracCY + vecY * left / cost;
}
