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

#include "defines.h"
#include "game-server/collisiondetection.hpp"
#include "game-server/deathlistener.hpp"
#include "game-server/mapcomposite.hpp"
#include "utils/logger.h"

Being::Being(int type, int id):
    MovingObject(type, id),
    mAction(STAND)
{
    mAttributes.resize(NB_ATTRIBUTES_BEING);
}

Being::~Being()
{
    // Notify death listeners
    DeathListeners::iterator i_end = mDeathListeners.end();
    DeathListeners::iterator i;
    for (i = mDeathListeners.begin(); i != i_end; ++i)
    {
        (*i)->deleted(this);
    }

}

int Being::damage(Damage damage)
{
    if (mAction == DEAD) return 0;

    // TODO: Implement dodge chance

    int HPloss = damage.value;

    // TODO: Implement elemental modifier

    switch (damage.type)
    {
        case DAMAGETYPE_PHYSICAL:
            HPloss -= getAttribute(DERIVED_ATTR_PHYSICAL_DEFENCE) / damage.piercing;
            HPloss -= getAttribute(ATTR_EFF_VITALITY);
            break;
        case DAMAGETYPE_MAGICAL:
            HPloss /= getAttribute(ATTR_EFF_WILLPOWER) + 1;
            break;
        case DAMAGETYPE_HAZARD:
            HPloss /= getAttribute(ATTR_EFF_VITALITY) + 1;
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

    return HPloss;
}

void Being::die()
{
    LOG_INFO("Being " << getPublicID() << " died");
    setAction(DEAD);
    // dead beings stay where they are
    clearDestination();

    // Notify death listeners
    DeathListeners::iterator i_end = mDeathListeners.end();
    DeathListeners::iterator i;
    for (i = mDeathListeners.begin(); i != i_end; ++i)
    {
        (*i)->died(this);
    }
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
    int SHORT_RANGE = 60;
    int SMALL_ANGLE = 35;
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

        if (type != OBJECT_CHARACTER && type != OBJECT_MONSTER) continue;

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

void Being::calculateDerivedAttributes()
{
    // effective values for basic attributes
    for (int i = NB_BASE_ATTRIBUTES; i < NB_EFFECTIVE_ATTRIBUTES; i++)
    {
        mAttributes.at(i)
            = getAttribute(i - NB_BASE_ATTRIBUTES); // TODO: add modifiers
    }

    // combat-related derived stats
    mAttributes.at(DERIVED_ATTR_HP_MAXIMUM)
        = getAttribute(ATTR_EFF_VITALITY); // TODO: find a better formula

    mAttributes.at(DERIVED_ATTR_PHYSICAL_ATTACK_MINIMUM)
        = getAttribute(ATTR_EFF_STRENGTH);

    mAttributes.at(DERIVED_ATTR_PHYSICAL_ATTACK_FLUCTUATION)
        = getAttribute(getWeaponStats().skill);

    mAttributes.at(DERIVED_ATTR_PHYSICAL_DEFENCE)
        = 0 /* + sum of equipment pieces */;
}

Damage Being::getPhysicalAttackDamage()
{
    Damage damage;
    WeaponStats weaponStats = getWeaponStats();

    damage.type = DAMAGETYPE_PHYSICAL;
    damage.value = getAttribute(DERIVED_ATTR_PHYSICAL_ATTACK_MINIMUM)
                + (rand()%getAttribute(DERIVED_ATTR_PHYSICAL_ATTACK_FLUCTUATION));
    damage.piercing = weaponStats.piercing;
    damage.element = weaponStats.element;
    damage.source = this;

    return damage;
}

WeaponStats Being::getWeaponStats()
{
    /* this function should never be called. it is just here to pacify the
     * compiler.
     */
    WeaponStats weaponStats;

    weaponStats.piercing = 1;
    weaponStats.element = ELEMENT_NEUTRAL;
    weaponStats.skill = 0;

    return weaponStats;
}
