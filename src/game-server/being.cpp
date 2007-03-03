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

#include "game-server/collisiondetection.hpp"
#include "game-server/mapcomposite.hpp"
#include "utils/logger.h"


void Being::damage(Damage damage)
{
    if (mAction == DEAD) return;

    int HPloss;

    HPloss = damage; // TODO: Implement complex damage calculation here

    if (HPloss > mHitpoints) HPloss = mHitpoints;

    mHitpoints -= HPloss;
    mHitsTaken.push_back(HPloss);
    LOG_INFO("Being " << getPublicID() << " got hit");

    if (mHitpoints == 0) die();
}

void Being::die()
{
    LOG_INFO("Being " << getPublicID() << " died");
    setAction(DEAD);
    // dead beings stay where they are
    setDestination(getPosition());
}

void Being::move()
{
    MovingObject::move();
    if (mAction == WALK || mAction == STAND)
    {
        if (mActionTime)
        {
            mAction = WALK;
        }
        else
        {
            mAction = STAND;
        }
    }
}

void Being::performAttack(MapComposite *map)
{
    int SHORT_RANGE = 32;
    int SMALL_ANGLE = 15;
    Point ppos = getPosition();
    int dir = getDirection();

    int attackAngle = 0;

    switch (dir)
    {
        case DIRECTION_UP:
           attackAngle = 90;
           break;
        case DIRECTION_DOWN:
           attackAngle = 270;
           break;
        case DIRECTION_LEFT:
           attackAngle = 180;
           break;
        case DIRECTION_RIGHT:
            attackAngle = 0;
            break;
        default:
            break;
    }

/* TODO: calculate real attack power and damage properties based on
             character equipment and stats. */
    Damage damage = 1;

    for (MovingObjectIterator i(map->getAroundObjectIterator(this, SHORT_RANGE)); i; ++i)
    {
        MovingObject *o = *i;
        if (o == this) continue;

        int type = o->getType();

        if (type != OBJECT_PLAYER && type != OBJECT_MONSTER) continue;

        Point opos = o->getPosition();

        if  (Collision::diskWithCircleSector(
                opos, o->getSize(),
                ppos, SHORT_RANGE, SMALL_ANGLE, attackAngle)
            )
        {
            static_cast< Being * >(o)->damage(damage);
        }
    }
}

void Being::setAction(Action action)
{
    mAction = action;
    if (action != Being::ATTACK && // The players are informed about these actions
        action != Being::WALK)     // by other messages
    {
        raiseUpdateFlags(UPDATEFLAG_ACTIONCHANGE);
    }
}
