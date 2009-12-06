/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
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

#include "game-server/actor.hpp"

#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"

#include <cassert>

void Actor::setPosition(const Point &p)
{
    mPos = p;

    // Update blockmap
    if (getMap())
    {
        const Point &oldP = getPosition();
        if ((oldP.x / 32 != p.x / 32 || oldP.y / 32 != p.y / 32))
        {
            getMap()->getMap()->freeTile(oldP.x / 32, oldP.y / 32, getBlockType());
            getMap()->getMap()->blockTile(p.x / 32, p.y / 32, getBlockType());
        }
    }
}

void Actor::setMap(MapComposite *map)
{
    assert (map);
    MapComposite *oldMap = getMap();
    Point p = getPosition();

    if (oldMap)
    {
        oldMap->getMap()->freeTile(p.x / 32, p.y / 32, getBlockType());
    }
    Thing::setMap(map);
    map->getMap()->blockTile(p.x / 32, p.y / 32, getBlockType());
    /* the last line might look illogical because the current position is
     * invalid on the new map, but it is necessary to block the old position
     * because the next call of setPosition() will automatically free the old
     * position. When we don't block the position now the occupation counting
     * will be off.
     */
}
