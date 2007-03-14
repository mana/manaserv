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
 *  $Id$
 */

#include "game-server/mapcomposite.hpp"
#include "game-server/object.hpp"
#include "game-server/state.hpp"
#include "game-server/trigger.hpp"

void WarpAction::process(Object *obj)
{
    if (obj->getType() == OBJECT_CHARACTER)
    {
        DelayedEvent e = { EVENT_WARP, mMap, mX, mY };
        gameState->enqueueEvent(obj, e);
    }
}

TriggerArea::TriggerArea(int map, Rectangle const &r, TriggerAction *ptr)
  : Thing(OBJECT_OTHER), mZone(r), mAction(ptr)
{
    setMapId(map);
}

void TriggerArea::update()
{
    MapComposite *map = gameState->getMap(getMapId());

    for (MovingObjectIterator i(map->getInsideRectangleIterator(mZone)); i; ++i)
    {
        if (mZone.contains((*i)->getPosition()))
        {
            mAction->process(*i);
        }
    }
}
