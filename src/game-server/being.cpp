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

    // TODO: Implement dodge chance

    int HPloss = damage.value;

    // TODO: Implement elemental modifier

    switch (damage.type)
    {
        case DAMAGETYPE_PHYSICAL:
            HPloss -= getRealStat(STAT_PHYSICAL_DEFENCE) / damage.penetration;
            break;
        case DAMAGETYPE_MAGICAL:
            // NIY
            break;
        case DAMAGETYPE_HAZARD:
            // NIY
            break;
        case DAMAGETYPE_OTHER:
            // nothing to do here
            break;
    }

    if (HPloss < 0) HPloss = 0;
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
    int SHORT_RANGE = 64;
    int SMALL_ANGLE = 45;
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
            static_cast< Being * >(o)->damage(getPhysicalAttackDamage());
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

void Being::addAbsoluteStatModifier(unsigned numStat, short value)
{
    mStats.absoluteModificator.at(numStat) = mStats.absoluteModificator.at(numStat) + value;
}

void Being::removeAbsoluteStatModifier(unsigned numStat, short value)
{
    mStats.absoluteModificator.at(numStat) = mStats.absoluteModificator.at(numStat) - value;
}

void Being::addPercentStatModifier(unsigned numStat, short value)
{
    if (value < -100)
    {
        LOG_WARN(   "Attempt to add a stat modificator for Being"<<
                    getPublicID()<<
                    "that would make the stat negative!"
                );
    }
    else
    {
        mStats.percentModificators.at(numStat).push_back(value);
    }
}

void Being::removePercentStatModifier(unsigned numStat, short value)
{
    std::list<short>::iterator i = mStats.percentModificators.at(numStat).begin();

    while (i != mStats.percentModificators.at(numStat).end())
    {
        if ((*i) = value)
        {
            mStats.percentModificators.at(numStat).erase(i);
            return;
        }
    }
    LOG_WARN(   "Attempt to remove a stat modificator for Being"<<
                getPublicID()<<
                "that hasn't been added before!"
            );
}

unsigned short Being::getRealStat(unsigned numStat)
{
    int value = mStats.base.at(numStat) + mStats.absoluteModificator.at(numStat);
    std::list<short>::iterator i;

    float multiplier = 1.0f;
    for (   i = mStats.percentModificators.at(numStat).begin();
            i != mStats.percentModificators.at(numStat).end();
            i++
        )
    {
        multiplier *= (100.0f + (float)(*i)) / 100.0f;
    }

    /* Floating point inaccuracies might result in a negative multiplier. That
     * would result in a stat near 2^16. To make sure that this doesn't happen
     * we return a value of 0 in that case
     */
    if (multiplier < 0.0f)
    {
        return 0;
    }
    else
    {
        return (unsigned short)(value * multiplier);
    }
}

Damage Being::getPhysicalAttackDamage()
{
    Damage damage;
    damage.type = DAMAGETYPE_PHYSICAL;
    damage.value = getRealStat(STAT_PHYSICAL_ATTACK_MINIMUM) + (rand()%getRealStat(STAT_PHYSICAL_ATTACK_FLUCTUATION));
    damage.penetration = 1; // TODO: get from equipped weapon
    damage.element = ELEMENT_NEUTRAL; // TODO: get from equipped weapon
    damage.source = this;

    return damage;
}
