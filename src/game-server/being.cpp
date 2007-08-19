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
    Attribute attr = { 0, 0 };
    mAttributes.resize(NB_BEING_ATTRIBUTES, attr);
    // Initialize element resistance to 100 (normal damage).
    for (int i = BASE_ELEM_BEGIN; i < BASE_ELEM_END; ++i)
    {
        mAttributes[i].base = 100;
    }
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

int Being::damage(Object *, Damage const &damage)
{
    if (mAction == DEAD) return 0;

    int HPloss = damage.base;
    if (damage.delta)
    {
        HPloss += rand() / (RAND_MAX / (damage.delta + 1));
    }

    /* Damage can either be avoided, or applied, or critical (applied twice).
       This is decided by comparing CTH and Evade. If they are equal, the
       probabilities are 10%, 80%, 10%. Otherwise, the bigger the CTH, the
       higher the chance to do a critical, up to 50%; and the bigger the Evade,
       the higher the chance to do evade the hit, up to 50% again. */

    int avoidChance = 10, criticalChance = 10;
    int diff = damage.cth - getModifiedAttribute(BASE_ATTR_EVADE);
    if (diff > 0)
    {
        // CTH - Evade >= 200 => 50% critical
        criticalChance += diff * diff / 1000;
        if (criticalChance > 50) criticalChance = 50;
    }
    else if (diff < 0)
    {
        // Evade - CTH >= 200 => 50% avoid
        avoidChance += diff * diff / 10000;
        if (avoidChance > 50) avoidChance = 50;
    }
    int chance = rand() / (RAND_MAX / 100);
    if (chance <= avoidChance)
    {
        mHitsTaken.push_back(0);
        return 0;
    }
    if (chance >= 100 - criticalChance) HPloss *= 2;

    /* Elemental modifier at 100 means normal damage. At 0, it means immune.
       And at 200, it means vulnerable (double damage). */
    int mod1 = getModifiedAttribute(BASE_ELEM_BEGIN + damage.element);

    /* Resistance to damage at 0 gives normal damage. At 100, it gives halved
       damage. At 200, it divides damage by 3. And so on. */
    int mod2 = 0;
    switch (damage.type)
    {
        case DAMAGE_PHYSICAL:
            mod2 = getModifiedAttribute(BASE_ATTR_PHY_RES);
            break;
        case DAMAGE_MAGICAL:
            mod2 = getModifiedAttribute(BASE_ATTR_MAG_RES);
            break;
        default:
            break;
    }
    HPloss = HPloss * mod1 / (100 + mod2);

    mHitsTaken.push_back(HPloss);
    LOG_DEBUG("Being " << getPublicID() << " got hit.");

    Attribute &HP = mAttributes[BASE_ATTR_HP];
    if (HPloss >= HP.base + HP.mod) HPloss = HP.base + HP.mod;
    if (HPloss > 0)
    {
        HP.mod -= HPloss;
        modifiedAttribute(BASE_ATTR_HP);
        if (HP.base + HP.mod == 0) die();
    }

    return HPloss;
}

void Being::die()
{
    if (mAction == DEAD) return;

    LOG_DEBUG("Being " << getPublicID() << " died.");
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

void Being::performAttack(Damage const &damage)
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

    for (MovingObjectIterator
         i(getMap()->getAroundObjectIterator(this, SHORT_RANGE)); i; ++i)
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
            static_cast< Being * >(o)->damage(this, damage);
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

void Being::addModifier(AttributeModifier const &mod)
{
    mModifiers.push_back(mod);
    mAttributes[mod.attr].mod += mod.value;
    modifiedAttribute(mod.attr);
}

void Being::removeEquipmentModifier(int attr, int value)
{
    bool found = false;
    for (AttributeModifiers::iterator i = mModifiers.begin(),
         i_end = mModifiers.end(); i != i_end; ++i)
    {
        found = i->level == 0 && i->attr == attr && i->value == value;
        if (found)
        {
            // Remove one equivalent modifier.
            mModifiers.erase(i);
            break;
        }
    }
    assert(found);
    mAttributes[attr].mod -= value;
    modifiedAttribute(attr);
}

void Being::dispellModifiers(int level)
{
    AttributeModifiers::iterator i = mModifiers.begin();
    while (i != mModifiers.end())
    {
        if (i->level && i->level <= level)
        {
            mAttributes[i->attr].mod -= i->value;
            modifiedAttribute(i->attr);
            i = mModifiers.erase(i);
            continue;
        }
        ++i;
    }
}

int Being::getModifiedAttribute(int attr) const
{
    int res = mAttributes[attr].base + mAttributes[attr].mod;
    return res <= 0 ? 0 : res;
}

void Being::update()
{
    // Update lifetime of effects.
    AttributeModifiers::iterator i = mModifiers.begin();
    while (i != mModifiers.end())
    {
        if (i->duration)
        {
            --i->duration;
            if (!i->duration)
            {
                i = mModifiers.erase(i);
                continue;
            }
        }
        ++i;
    }
}
