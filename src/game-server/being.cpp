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

#include <cassert>

#include "game-server/being.hpp"

#include "defines.h"
#include "game-server/collisiondetection.hpp"
#include "game-server/eventlistener.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/effect.hpp"
#include "game-server/statuseffect.hpp"
#include "game-server/statusmanager.hpp"
#include "utils/logger.h"

Being::Being(ThingType type):
    Actor(type),
    mAction(STAND),
    mTarget(NULL),
    mSpeed(0),
    mDirection(0)
{
    Attribute attr = { 0, 0 };
    mAttributes.resize(NB_BEING_ATTRIBUTES + CHAR_ATTR_NB, attr);
    // Initialize element resistance to 100 (normal damage).
    for (int i = BASE_ELEM_BEGIN; i < BASE_ELEM_END; ++i)
    {
        mAttributes[i].base = 100;
    }
}

int Being::damage(Actor *, const Damage &damage)
{
    if (mAction == DEAD)
        return 0;

    int HPloss = damage.base;
    if (damage.delta)
    {
        HPloss += rand() / (RAND_MAX / (damage.delta + 1));
    }

    int hitThrow = rand()%(damage.cth + 1);
    int evadeThrow = rand()%(getModifiedAttribute(BASE_ATTR_EVADE) + 1);
    if (evadeThrow > hitThrow)
    {
        HPloss = 0;
    }

    /* Elemental modifier at 100 means normal damage. At 0, it means immune.
       And at 200, it means vulnerable (double damage). */
    int mod1 = getModifiedAttribute(BASE_ELEM_BEGIN + damage.element);
    HPloss = HPloss * (mod1 / 100);
    /* Defence is an absolute value which is subtracted from the damage total. */
    int mod2 = 0;
    switch (damage.type)
    {
        case DAMAGE_PHYSICAL:
            mod2 = getModifiedAttribute(BASE_ATTR_PHY_RES);
            HPloss = HPloss - mod2;
            break;
        case DAMAGE_MAGICAL:
            mod2 = getModifiedAttribute(BASE_ATTR_MAG_RES);
            HPloss = HPloss / (mod2 + 1);
            break;
        default:
            break;
    }

    if (HPloss > 0)
    {
        mHitsTaken.push_back(HPloss);
        Attribute &HP = mAttributes[BASE_ATTR_HP];
        LOG_DEBUG("Being " << getPublicID() << " suffered "<<HPloss<<" damage. HP: "<<HP.base + HP.mod<<"/"<<HP.base);
        HP.mod -= HPloss;
        modifiedAttribute(BASE_ATTR_HP);
        setTimerSoft(T_B_HP_REGEN, 50); // no HP regen for 5 seconds after being hit
    } else {
        HPloss = 0;
    }

    return HPloss;
}

void Being::died()
{
    if (mAction == DEAD)
        return;

    LOG_DEBUG("Being " << getPublicID() << " died.");
    setAction(DEAD);
    // dead beings stay where they are
    clearDestination();

    // reset target
    mTarget = NULL;

    for (Listeners::iterator i = mListeners.begin(),
         i_end = mListeners.end(); i != i_end;)
    {
        const EventListener &l = **i;
        ++i; // In case the listener removes itself from the list on the fly.
        if (l.dispatch->died) l.dispatch->died(&l, this);
    }
}

void Being::setDestination(const Point &dst)
{
    mDst = dst;
    raiseUpdateFlags(UPDATEFLAG_NEW_DESTINATION);
    mPath.clear();
}

Path Being::findPath()
{
    mOld = getPosition();
    int startX = mOld.x / 32, startY = mOld.y / 32;
    int destX = mDst.x / 32, destY = mDst.y / 32;
    Map *map = getMap()->getMap();
    return map->findPath(startX, startY, destX, destY, getWalkMask());
}

void Being::setSpeed(float s)
{
  if (s > 0)
      mSpeed = (int)(32000 / (s * (float)DEFAULT_TILE_LENGTH));
  else
      mSpeed = 0;
}

void Being::move()
{
    // Don't deal with not moving beings
    if (mSpeed <= 0 && mSpeed >= 32000)
          return;

    mOld = getPosition();

    if (mActionTime > 100)
    {
        // Current move has not yet ended
        mActionTime -= 100;
        return;
    }

    int tileSX = mOld.x / 32, tileSY = mOld.y / 32;
    int tileDX = mDst.x / 32, tileDY = mDst.y / 32;
    if (tileSX == tileDX && tileSY == tileDY)
    {
        // Moving while staying on the same tile is free
        setPosition(mDst);
        mActionTime = 0;
        return;
    }

    Map *map = getMap()->getMap();

    /* If no path exists, the for-loop won't be entered. Else a path for the
     * current destination has already been calculated.
     * The tiles in this path have to be checked for walkability,
     * in case there have been changes. The 'getWalk' method of the Map
     * class has been used, because that seems to be the most logical
     * place extra functionality will be added.
     */
    for (PathIterator pathIterator = mPath.begin();
            pathIterator != mPath.end(); pathIterator++)
    {
        if (!map->getWalk(pathIterator->x, pathIterator->y, getWalkMask()))
        {
            mPath.clear();
            break;
        }
    }

    if (mPath.empty())
    {
        // No path exists: the walkability of cached path has changed, the
        // destination has changed, or a path was never set.
        mPath = findPath();
    }

    if (mPath.empty())
    {
        // no path was found
        mDst = mOld;
        mActionTime = 0;
        return;
    }

    Position prev(tileSX, tileSY);
    Point pos;
    do
    {
        Position next = mPath.front();
        mPath.pop_front();
        // 362 / 256 is square root of 2, used for walking diagonally
        mActionTime += (prev.x != next.x && prev.y != next.y)
                       ? mSpeed * 362 / 256 : mSpeed;
        if (mPath.empty())
        {
            // skip last tile center
            pos = mDst;
            break;
        }
        // position the actor in the middle of the tile for pathfinding purposes
        pos.x = next.x * 32 + 16;
        pos.y = next.y * 32 + 16;
    }
    while (mActionTime < 100);
    setPosition(pos);

    mActionTime = mActionTime > 100 ? mActionTime - 100 : 0;

    if (mAction == WALK || mAction == STAND)
    {
        mAction = (mActionTime) ? WALK : STAND;
    }
}

int Being::directionToAngle(int direction)
{
    switch (direction)
    {
        case DIRECTION_UP:    return  90;
        case DIRECTION_DOWN:  return 270;
        case DIRECTION_RIGHT: return 180;
        case DIRECTION_LEFT:
        default:              return   0;
    }
}

int Being::performAttack(Being *target, unsigned range, const Damage &damage)
{
    // check target legality
    if (!target || target == this || target->getAction() == Being::DEAD || !target->canFight())
            return -1;
    if (getMap()->getPvP() == PVP_NONE && target->getType() == OBJECT_CHARACTER &&
        getType() == OBJECT_CHARACTER)
        return -1;

    // check if target is in range using the pythagorean theorem
    int distx = this->getPosition().x - target->getPosition().x;
    int disty = this->getPosition().y - target->getPosition().y;
    int distSquare = (distx * distx + disty * disty);
    int maxDist = range + target->getSize();
    if (maxDist * maxDist < distSquare)
        return -1;

    mActionTime += 1000; // set to 10 ticks wait time

    return (mTarget->damage(this, damage));
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

void Being::applyModifier(int attr, int amount, int duration, int lvl)
{
    if (duration)
    {
        AttributeModifier mod;
        mod.attr = attr;
        mod.value = amount;
        mod.duration = duration;
        mod.level = lvl;
        mModifiers.push_back(mod);
    }
    mAttributes[attr].mod += amount;
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

void Being::applyStatusEffect(int id, int timer)
{
    if (mAction == DEAD)
        return;

    if (StatusEffect *statusEffect = StatusManager::getStatus(id))
    {
        Status newStatus;
        newStatus.status = statusEffect;
        newStatus.time = timer;
        mStatus[id] = newStatus;
    }
    else
    {
        LOG_ERROR("No status effect with ID " << id);
    }
}

void Being::removeStatusEffect(int id)
{
    setStatusEffectTime(id, 0);
}

bool Being::hasStatusEffect(int id) const
{
    StatusEffects::const_iterator it = mStatus.begin();
    while (it != mStatus.end())
    {
        if (it->second.status->getId() == id)
            return true;
        it++;
    }
    return false;
}

unsigned Being::getStatusEffectTime(int id) const
{
    StatusEffects::const_iterator it = mStatus.find(id);
    if (it != mStatus.end()) return it->second.time;
    else return 0;
}

void Being::setStatusEffectTime(int id, int time)
{
    StatusEffects::iterator it = mStatus.find(id);
    if (it != mStatus.end()) it->second.time = time;
}

void Being::update()
{
    //update timers
    for (Timers::iterator i = mTimers.begin(); i != mTimers.end(); i++)
    {
        if (i->second > -1) i->second--;
    }

    int oldHP = getModifiedAttribute(BASE_ATTR_HP);
    int newHP = oldHP;
    int maxHP = getAttribute(BASE_ATTR_HP);

    // Regenerate HP
    if (mAction != DEAD && !isTimerRunning(T_B_HP_REGEN))
    {
        setTimerHard(T_B_HP_REGEN, TICKS_PER_HP_REGENERATION);
        newHP += getModifiedAttribute(BASE_ATTR_HP_REGEN);
    }
    // Cap HP at maximum
    if (newHP > maxHP)
    {
        newHP = maxHP;
    }
    // Only update HP when it actually changed to avoid network noise
    if (newHP != oldHP)
    {
        applyModifier(BASE_ATTR_HP, newHP - oldHP);
        raiseUpdateFlags(UPDATEFLAG_HEALTHCHANGE);
    }

    // Update lifetime of effects.
    AttributeModifiers::iterator i = mModifiers.begin();
    while (i != mModifiers.end())
    {
        --i->duration;
        if (!i->duration)
        {
            mAttributes[i->attr].mod -= i->value;
            modifiedAttribute(i->attr);
            i = mModifiers.erase(i);
            continue;
        }
        ++i;
    }

    // Update and run status effects
    StatusEffects::iterator it = mStatus.begin();
    while (it != mStatus.end())
    {
        it->second.time--;
        if (it->second.time > 0 && mAction != DEAD)
        {
            it->second.status->tick(this, it->second.time);
        }

        if (it->second.time <= 0 || mAction == DEAD)
        {
            mStatus.erase(it);
        }
        it++;
    }

    // Check if being died
    if (getModifiedAttribute(BASE_ATTR_HP) <= 0 && mAction != DEAD)
    {
        died();
    }
}

void Being::setTimerSoft(TimerID id, int value)
{
    Timers::iterator i = mTimers.find(id);
    if (i == mTimers.end())
    {
        mTimers[id] = value;
    }
    else if (i->second < value)
    {
        i->second = value;
    }
}

void Being::setTimerHard(TimerID id, int value)
{
    mTimers[id] = value;
}

int Being::getTimer(TimerID id) const
{
    Timers::const_iterator i = mTimers.find(id);
    return (i == mTimers.end()) ? -1 : i->second;
}

bool Being::isTimerRunning(TimerID id) const
{
    return getTimer(id) > 0;
}

bool Being::isTimerJustFinished(TimerID id) const
{
    return getTimer(id) == 0;
}
