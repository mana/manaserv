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

#include "game-server/monster.hpp"

#include "game-server/item.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/state.hpp"
#include "utils/logger.h"

ItemClass *MonsterClass::getRandomDrop() const
{
    int p = rand() / (RAND_MAX / 10000);
    for (MonsterDrops::const_iterator i = mDrops.begin(),
         i_end = mDrops.end(); i != i_end; ++i)
    {
        p -= i->probability;
        if (p < 0)
        {
            return i->item;
        }
    }
    return NULL;
}

Monster::Monster(MonsterClass *specy):
    Being(OBJECT_MONSTER, 65535),
    mSpecy(specy),
    mCountDown(0),
    mAttackTime(0),
    mAttackPreDelay(5),
    mAttackAftDelay(10)
{
    LOG_DEBUG("Monster spawned!");
    mAgressive = false;   // TODO: Get from monster database
    mAgressionRange = 10; // TODO: Get from monster database
    // TODO: Fill with the real attributes
    mAttributes.resize(NB_ATTRIBUTES_CONTROLLED, 1);

    // Some bogus values for testing monster attacks on players
    setAttribute(BASE_ATTR_STRENGTH, 10);
    setAttribute(MONSTER_SKILL_WEAPON, 3);

    // Set positions relative to target from which the monster can attack
    mAttackPositions.push_back(AttackPosition(+32, 0, DIRECTION_LEFT));
    mAttackPositions.push_back(AttackPosition(-32, 0, DIRECTION_RIGHT));
    mAttackPositions.push_back(AttackPosition(0, +32, DIRECTION_DOWN));
    mAttackPositions.push_back(AttackPosition(0, -32, DIRECTION_UP));
}

Monster::~Monster()
{
    // deactivate death listeners
    std::map<Being *, int>::iterator i;
    for (i = mAnger.begin(); i != mAnger.end(); i++)
    {
        i->first->removeDeathListener(this);
    }
}

void Monster::update()
{
    // If dead do nothing but rot
    if (mAction == DEAD)
    {
        mCountDown--;
        if (mCountDown <= 0)
        {
            raiseUpdateFlags(UPDATEFLAG_REMOVE);
        }
        return;
    }

    // If currently attacking finish attack;
    if (mAttackTime)
    {
        if (mAttackTime == mAttackAftDelay)
        {
            mAction = ATTACK;
            raiseUpdateFlags(UPDATEFLAG_ATTACK);
        }
        mAttackTime--;
        return;
    }

    // Check potential attack positions
    Being *bestAttackTarget = NULL;
    int bestTargetPriority = 0;
    Point bestAttackPosition;
    Direction bestAttackDirection = DIRECTION_DOWN;

    // Iterate through objects nearby
    for (MovingObjectIterator i(getMap()->getAroundCharacterIterator(this, AROUND_AREA)); i; ++i)
    {
        // We only want to attack player characters
        if ((*i)->getType() != OBJECT_CHARACTER) continue;

        Being *target = static_cast<Being *> (*i);

        // Dead characters are ignored
        if (target->getAction() == DEAD) continue;

        // Determine how much we hate the target
        int targetPriority = 0;
        std::map<Being *, int, std::greater<Being *> >::iterator angerIterator;
        angerIterator = mAnger.find(target);
        if (angerIterator != mAnger.end())
        {
            targetPriority = angerIterator->second;
        }
        else if (mAgressive)
        {
            targetPriority = 1;
        }
        else
        {
            continue;
        }

        // Check all attack positions
        for (std::list<AttackPosition>::iterator j = mAttackPositions.begin();
             j != mAttackPositions.end();
             j++)
        {
            Point attackPosition = (*i)->getPosition();
            attackPosition.x += (*j).x;
            attackPosition.y += (*j).y;

            int posPriority = calculatePositionPriority(attackPosition,
                                                        targetPriority);
            if (posPriority > bestTargetPriority)
            {
                bestAttackTarget = target;
                bestTargetPriority = posPriority;
                bestAttackPosition = attackPosition;
                bestAttackDirection = (*j).direction;
            }
        }
    }

    // Check if an attack position has been found
    if (bestAttackTarget)
    {
        // Check if we are there
        if (bestAttackPosition == getPosition())
        {
            // We are there - let's get ready to beat the crap out of the target
            setDirection(bestAttackDirection);
            mAttackTime = mAttackPreDelay + mAttackAftDelay;
        }
        else
        {
            // We aren't there yet - let's move
            setDestination(bestAttackPosition);
        }
    }
    else
    {
        // We have no target - let's wander around
        mCountDown--;
        if (mCountDown <= 0)
        {
            Point randomPos(rand() % 160 - 80 + getPosition().x,
                            rand() % 160 - 80 + getPosition().y);
            setDestination(randomPos);
            mCountDown = 10 + rand() % 10;
        }
    }
}

int Monster::calculatePositionPriority(Point position, int targetPriority)
{
    Point thisPos = getPosition();

    // Check if we already are on this position
    if (thisPos.x / 32 == position.x / 32 &&
        thisPos.y / 32 == position.y / 32)
    {
        return targetPriority *= mAgressionRange;
    }

    std::list<PATH_NODE> path;
    path = getMap()->getMap()->findPath(thisPos.x / 32, thisPos.y / 32,
                                        position.x / 32, position.y / 32,
                                        mAgressionRange);

    if (path.empty() || path.size() >= mAgressionRange)
    {
        return 0;
    }
    else
    {
        return targetPriority * (mAgressionRange - path.size());
    }
}

void Monster::died(Being *being)
{
    mAnger.erase(being);
    mDeathListeners.remove((DeathListener *)being);
}

int Monster::damage(Damage damage)
{
    int HPLoss = Being::damage(damage);
    if  (   HPLoss
         && damage.source
         && damage.source->getType() == OBJECT_CHARACTER
        )
    {
        if (mAnger.find(damage.source) == mAnger.end())
        {
            damage.source->addDeathListener(this);
            mAnger[damage.source] = HPLoss;
        }
        else
        {
            mAnger[damage.source] += HPLoss;
        }
    }
    return HPLoss;
}

void Monster::die()
{
    mCountDown = 50; // Sets remove time to 5 seconds
    Being::die();
    if (ItemClass *drop = mSpecy->getRandomDrop())
    {
        Item *item = new Item(drop, 1);
        item->setMap(getMap());
        item->setPosition(getPosition());
        DelayedEvent e = { EVENT_INSERT };
        GameState::enqueueEvent(item, e);
    }
}

WeaponStats Monster::getWeaponStats()
{

    WeaponStats weaponStats;

    /*
     * TODO: This should all be set by the monster database
     */
    weaponStats.piercing = 1;
    weaponStats.element = ELEMENT_NEUTRAL;
    weaponStats.skill = MONSTER_SKILL_WEAPON;

    return weaponStats;
}

void Monster::calculateDerivedAttributes()
{
    Being::calculateDerivedAttributes();
    /*
     * Do any monster specific attribute calculation here
     */
}
