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

#include "being.h"
#include "mapcomposite.h"
#include "utils/logger.h"

void Being::damage(Damage damage)
{
    int HPloss;

    HPloss = damage; // TODO: Implement complex damage calculation here

    mHitpoints -= HPloss;
    mHitsTaken.push_back(HPloss);
    LOG_DEBUG("Being " << getPublicID() << " got hit", 0);
}

void Being::performAttack(MapComposite* map)
{
    std::list<ObjectPtr> victimList;
    std::list<Point> attackZone;

    Point attackPoint = getPosition();

    unsigned char direction = getDirection();
    if (direction & UP)
    {
        attackPoint.y -= 32;
        attackPoint.x -= 32;
        attackZone.push_back(attackPoint);
        attackPoint.x += 32;
        attackZone.push_back(attackPoint);
        attackPoint.x += 32;
        attackZone.push_back(attackPoint);
    }
    else if (direction & RIGHT)
    {
        attackPoint.x += 32;
        attackPoint.y -= 32;
        attackZone.push_back(attackPoint);
        attackPoint.y += 32;
        attackZone.push_back(attackPoint);
        attackPoint.y += 32;
        attackZone.push_back(attackPoint);
    }
    else if (direction & DOWN)
    {
        attackPoint.y += 32;
        attackPoint.x -= 32;
        attackZone.push_back(attackPoint);
        attackPoint.x += 32;
        attackZone.push_back(attackPoint);
        attackPoint.x += 32;
        attackZone.push_back(attackPoint);
    }
    else {
        attackPoint.x -= 32;
        attackPoint.y -= 32;
        attackZone.push_back(attackPoint);
        attackPoint.y += 32;
        attackZone.push_back(attackPoint);
        attackPoint.y += 32;
        attackZone.push_back(attackPoint);
    }

    attackZone.push_back(attackPoint);  // point player is facing

    // get enemies to hurt
    for (std::list<Point>::iterator i = attackZone.begin(),
         i_end = attackZone.end(); i != i_end; ++i)
    {
        std::list<ObjectPtr> newVictimList = map->getObjectsOnTile((*i));
        victimList.splice(victimList.end(), newVictimList);
    }

    // apply damage to victims
    Damage damage;

    /* TODO:    calculate real attack power and damage properties based on
     *          character equipment and stats
     */
    damage = 1;

    for (std::list<ObjectPtr>::iterator i = victimList.begin(),
         i_end = victimList.end(); i != i_end; ++i)
    {
        if ((*i)->getType() == OBJECT_PLAYER || (*i)->getType() == OBJECT_MONSTER)
        {
            static_cast<Being*>(&**i)->damage(damage);
        }
    }
}
