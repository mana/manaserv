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

#include "game-server/being.hpp"
#include "game-server/mapcomposite.hpp"
#include "utils/logger.h"

void Being::damage(Damage damage)
{
    int HPloss;

    HPloss = damage; // TODO: Implement complex damage calculation here

    mHitpoints -= HPloss;
    mHitsTaken.push_back(HPloss);
    LOG_DEBUG("Being " << getPublicID() << " got hit", 0);
}

void Being::performAttack(MapComposite *map)
{
    int SHORT_RANGE = 32;
    Point ppos = getPosition();
    int dir = getDirection();

    /* TODO: calculate real attack power and damage properties based on
             character equipment and stats. */
    Damage damage = 1;

    for (MovingObjectIterator i(map->getAroundObjectIterator(this, SHORT_RANGE)); i; ++i)
    {
        MovingObject *o = *i;
        if (o == this)
        {
            continue;
        }

        int type = o->getType();
        Point opos = o->getPosition();
        int dx = opos.x - ppos.x, dy = opos.y - ppos.y;

        if ((type != OBJECT_PLAYER && type != OBJECT_MONSTER) ||
            (std::abs(dx) > SHORT_RANGE || std::abs(dy) > SHORT_RANGE))
        {
            continue;
        }

        // basic triangle-shaped damage zone
        switch (dir)
        {
            case DIRECTION_UP:
                if (!(dy <= dx && dx <= -dy)) continue;
                break;
            case DIRECTION_DOWN:
                if (!(-dy <= dx && dx <= dy)) continue;
                break;
            case DIRECTION_LEFT:
                if (!(dx <= dy && dy <= -dx)) continue;
                break;
            case DIRECTION_RIGHT:
                if (!(-dx <= dy && dy <= dx)) continue;
                break;
            default:
                break;
        }

        static_cast< Being * >(o)->damage(damage);
    }
}
