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

#include "game-server/actor.h"

#include "game-server/map.h"
#include "game-server/mapcomposite.h"

#include <cassert>

void Actor::setPosition(const Point &p)
{
    mPos = p;

    // Update blockmap
    if (getMap())
    {
        Map *map = getMap()->getMap();
        int tileWidth = map->getTileWidth();
        int tileHeight = map->getTileHeight();
        const Point &oldP = getPosition();
        if ((oldP.x / tileWidth != p.x / tileWidth
            || oldP.y / tileHeight != p.y / tileHeight))
        {
            map->freeTile(oldP.x / tileWidth, oldP.y / tileHeight,
                          getBlockType());
            map->blockTile(p.x / tileWidth, p.y / tileHeight, getBlockType());
        }
    }
}

void Actor::setMap(MapComposite *mapComposite)
{
    assert (mapComposite);
    MapComposite *oldMapComposite = getMap();
    Point p = getPosition();

    if (oldMapComposite)
    {
        Map *oldMap = oldMapComposite->getMap();
        int oldTileWidth = oldMap->getTileWidth();
        int oldTileHeight = oldMap->getTileHeight();
        oldMap->freeTile(p.x / oldTileWidth, p.y / oldTileHeight,
                         getBlockType());
    }
    Thing::setMap(mapComposite);
    Map *map = mapComposite->getMap();
    int tileWidth = map->getTileWidth();
    int tileHeight = map->getTileHeight();
    map->blockTile(p.x / tileWidth, p.y / tileHeight, getBlockType());
    /* the last line might look illogical because the current position is
     * invalid on the new map, but it is necessary to block the old position
     * because the next call of setPosition() will automatically free the old
     * position. When we don't block the position now the occupation counting
     * will be off.
     */
}
