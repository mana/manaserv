/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
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
#include "common/configuration.hpp"
#include "game-server/attributemanager.hpp"
#include "game-server/character.hpp"
#include "game-server/collisiondetection.hpp"
#include "game-server/eventlistener.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/effect.hpp"
#include "game-server/statuseffect.hpp"
#include "game-server/statusmanager.hpp"
#include "utils/logger.h"
#include "utils/speedconv.hpp"

Being::Being(ThingType type):
    Actor(type),
    mAction(STAND),
    mTarget(NULL),
    mDirection(0)
{
    const AttributeScopes &attr = attributeManager->getAttributeInfoForType(ATTR_BEING);
    LOG_DEBUG("Being creation: initialisation of " << attr.size() << " attributes.");
    for (AttributeScopes::const_iterator it1 = attr.begin(),
         it1_end = attr.end();
        it1 != it1_end;
        ++it1)
    {
        if (mAttributes.count(it1->first))
            LOG_WARN("Redefinition of attribute '" << it1->first << "'!");
        LOG_DEBUG("Attempting to create attribute '" << it1->first << "'.");
        mAttributes.insert(std::make_pair(it1->first,
                                          Attribute(*it1->second)));

    }
    // TODO: Way to define default base values?
    // Should this be handled by the virtual modifiedAttribute?
    // URGENT either way
#if 0
    // Initialize element resistance to 100 (normal damage).
    for (i = BASE_ELEM_BEGIN; i < BASE_ELEM_END; ++i)
    {
        mAttributes[i] = Attribute(TY_ST);
        mAttributes[i].setBase(100);
    }
#endif
}

int Being::damage(Actor *source, const Damage &damage)
{
    if (mAction == DEAD)
        return 0;

    int HPloss = damage.base;
    if (damage.delta)
        HPloss += rand() * (damage.delta + 1) / RAND_MAX;

    // TODO magical attacks and associated elemental modifiers
    switch (damage.type)
    {
        case DAMAGE_PHYSICAL:
            if (!damage.trueStrike &&
                rand()%((int) getModifiedAttribute(ATTR_DODGE) + 1) >
                    rand()%(damage.cth + 1))
            {
                HPloss = 0;
                // TODO Process triggers for a dodged physical attack here.
                // If there is an attacker included, also process triggers for the attacker (failed physical strike)
            }
            else
            {
                HPloss = HPloss * (1.0 - (0.0159375f *
                                          getModifiedAttribute(ATTR_DEFENSE)) /
                                   (1.0 + 0.017 *
                                    getModifiedAttribute(ATTR_DEFENSE))) +
                         (rand()%((HPloss >> 4) + 1));
                // TODO Process triggers for receiving damage here.
                // If there is an attacker included, also process triggers for the attacker (successful physical strike)
            }
            break;
        case DAMAGE_MAGICAL:
#if 0
            getModifiedAttribute(BASE_ELEM_BEGIN + damage.element);
#else
            LOG_WARN("Attempt to use magical type damage! This has not been"
                      "implemented yet and should not be used!");
            HPloss = 0;
#endif
        case DAMAGE_DIRECT:
            break;
        default:
            LOG_WARN("Unknown damage type '" << damage.type << "'!");
            break;
    }

    if (HPloss > 0)
    {
        mHitsTaken.push_back(HPloss);
        Attribute &HP = mAttributes.at(ATTR_HP);
        LOG_DEBUG("Being " << getPublicID() << " suffered " << HPloss
                  << " damage. HP: "
                  << HP.getModifiedAttribute() << "/"
                  << mAttributes.at(ATTR_MAX_HP).getModifiedAttribute());
        setAttribute(ATTR_HP, HP.getBase() - HPloss);
        // No HP regen after being hit if this is set.
        setTimerSoft(T_B_HP_REGEN,
                     Configuration::getValue("game_hpRegenBreakAfterHit", 0));
    }
    else
    {
        HPloss = 0;
    }

    return HPloss;
}

void Being::heal()
{
    Attribute &hp = mAttributes.at(ATTR_HP);
    Attribute &maxHp = mAttributes.at(ATTR_MAX_HP);
    if (maxHp.getModifiedAttribute() == hp.getModifiedAttribute())
        return; // Full hp, do nothing.

    // Reset all modifications present in hp.
    hp.clearMods();
    setAttribute(ATTR_HP, maxHp.getModifiedAttribute());
}

void Being::heal(int gain)
{
    Attribute &hp = mAttributes.at(ATTR_HP);
    Attribute &maxHp = mAttributes.at(ATTR_MAX_HP);
    if (maxHp.getModifiedAttribute() == hp.getModifiedAttribute())
        return; // Full hp, do nothing.

    // Cannot go over maximum hitpoints.
    setAttribute(ATTR_HP, hp.getBase() + gain);
    if (hp.getModifiedAttribute() > maxHp.getModifiedAttribute())
        setAttribute(ATTR_HP, maxHp.getModifiedAttribute());
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
    Map *map = getMap()->getMap();
    int tileWidth = map->getTileWidth();
    int tileHeight = map->getTileHeight();
    int startX = mOld.x / tileWidth, startY = mOld.y / tileHeight;
    int destX = mDst.x / tileWidth, destY = mDst.y / tileHeight;

    return map->findPath(startX, startY, destX, destY, getWalkMask());
}

void Being::move()
{
    // Immobile beings cannot move.
    if (!checkAttributeExists(ATTR_MOVE_SPEED_RAW)
        || !getModifiedAttribute(ATTR_MOVE_SPEED_RAW))
          return;

    mOld = getPosition();

    if (mActionTime > 100)
    {
        // Current move has not yet ended
        mActionTime -= 100;
        return;
    }

    Map *map = getMap()->getMap();
    int tileWidth = map->getTileWidth();
    int tileHeight = map->getTileHeight();
    int tileSX = mOld.x / tileWidth, tileSY = mOld.y / tileHeight;
    int tileDX = mDst.x / tileWidth, tileDY = mDst.y / tileHeight;

    if (tileSX == tileDX && tileSY == tileDY)
    {
        if (mAction == WALK)
            setAction(STAND);
        // Moving while staying on the same tile is free
        setPosition(mDst);
        mActionTime = 0;
        return;
    }

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
        if (mAction == WALK)
            setAction(STAND);
        // no path was found
        mDst = mOld;
        mActionTime = 0;
        return;
    }

    setAction(WALK);

    Position prev(tileSX, tileSY);
    Point pos;
    do
    {
        Position next = mPath.front();
        mPath.pop_front();
        // SQRT2 is used for diagonal movement.
        mActionTime += (prev.x == next.x || prev.y == next.y) ?
                       getModifiedAttribute(ATTR_MOVE_SPEED_RAW) :
                       getModifiedAttribute(ATTR_MOVE_SPEED_RAW) * SQRT2;
        if (mPath.empty())
        {
            // skip last tile center
            pos = mDst;
            break;
        }

        // Position the actor in the middle of the tile for pathfinding purposes
        pos.x = next.x * tileWidth + (tileWidth / 2);
        pos.y = next.y * tileHeight + (tileHeight / 2);
    }
    while (mActionTime < 100);
    setPosition(pos);

    mActionTime = mActionTime > 100 ? mActionTime - 100 : 0;
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

int Being::performAttack(Being *target, const Damage &damage) {
    return performAttack(target, damage.range, damage);
}

int Being::performAttack(Being *target, unsigned range, const Damage &damage)
{
    // check target legality
    if (!target || target == this || target->getAction() == Being::DEAD
        || !target->canFight())
            return -1;
    if (getMap()->getPvP() == PVP_NONE && target->getType() == OBJECT_CHARACTER
        && getType() == OBJECT_CHARACTER)
        return -1;

    // check if target is in range using the pythagorean theorem
    int distx = this->getPosition().x - target->getPosition().x;
    int disty = this->getPosition().y - target->getPosition().y;
    int distSquare = (distx * distx + disty * disty);
    int maxDist = range + target->getSize();
    if (maxDist * maxDist < distSquare)
        return -1;

    //mActionTime += 1000; // No tick. Auto-attacks should have their own, built-in delays.

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

void Being::applyModifier(unsigned int attr, double value, unsigned int layer,
                          unsigned int duration, unsigned int id)
{
    mAttributes.at(attr).add(duration, value, layer, id);
    updateDerivedAttributes(attr);
}

bool Being::removeModifier(unsigned int attr, double value, unsigned int layer,
                           unsigned int id, bool fullcheck)
{
    bool ret = mAttributes.at(attr).remove(value, layer, id, fullcheck);
    updateDerivedAttributes(attr);
    return ret;
}

void Being::setAttribute(unsigned int id, double value)
{
    AttributeMap::iterator ret = mAttributes.find(id);
    if (ret == mAttributes.end())
    {
        /*
         * The attribute does not yet exist, so we must attempt to create it.
         */
        LOG_ERROR("Being: Attempt to access non-existing attribute '"
                  << id << "'!");
        LOG_WARN("Being: Creation of new attributes dynamically is not "
                 "implemented yet!");
    }
    else
    {
        ret->second.setBase(value);
        updateDerivedAttributes(id);
    }
}

double Being::getAttribute(unsigned int id) const
{
    AttributeMap::const_iterator ret = mAttributes.find(id);
    if (ret == mAttributes.end()) return 0;
    return ret->second.getBase();
}


double Being::getModifiedAttribute(unsigned int id) const
{
    AttributeMap::const_iterator ret = mAttributes.find(id);
    if (ret == mAttributes.end()) return 0;
    return ret->second.getModifiedAttribute();
}

void Being::setModAttribute(unsigned int id, double value)
{
    // No-op to satisfy shared structure.
    // The game-server calculates this manually.
    return;
}

bool Being::recalculateBaseAttribute(unsigned int attr)
{
    LOG_DEBUG("Received update attribute recalculation request at Being for "
              << attr << ".");
    if (!mAttributes.count(attr)) return false;
    double newBase = getAttribute(attr);

    switch (attr)
    {
    case ATTR_HP_REGEN:
        {
            double hpPerSec = getModifiedAttribute(ATTR_VIT) * 0.05;
            newBase = (hpPerSec * TICKS_PER_HP_REGENERATION / 10);
        }
        break;
    case ATTR_HP:
        double diff;
        if ((diff = getModifiedAttribute(ATTR_HP)
            - getModifiedAttribute(ATTR_MAX_HP)) > 0)
            newBase -= diff;
        break;
    case ATTR_MAX_HP:
        newBase = ((getModifiedAttribute(ATTR_VIT) + 3)
                   * (getModifiedAttribute(ATTR_VIT) + 20)) * 0.125;
        break;
    case ATTR_MOVE_SPEED_TPS:
        newBase = 3.0 + getModifiedAttribute(ATTR_AGI) * 0.08; // Provisional.
        break;
    case ATTR_MOVE_SPEED_RAW:
        newBase = utils::tpsToSpeed(getModifiedAttribute(ATTR_MOVE_SPEED_TPS));
        break;
    case ATTR_INV_CAPACITY:
        // Provisional
        newBase = 2000.0 + getModifiedAttribute(ATTR_STR) * 180.0;
        break;
    }
    if (newBase != getAttribute(attr))
    {
        setAttribute(attr, newBase);
        updateDerivedAttributes(attr);
        return true;
    }
    LOG_DEBUG("No changes to sync for attribute '" << attr << "'.");
    return false;
}

void Being::updateDerivedAttributes(unsigned int attr)
{
    switch(attr)
    {
    case ATTR_MAX_HP:
        updateDerivedAttributes(ATTR_HP);
    case ATTR_HP:
        raiseUpdateFlags(UPDATEFLAG_HEALTHCHANGE);
        break;
    case ATTR_MOVE_SPEED_TPS:
        updateDerivedAttributes(ATTR_MOVE_SPEED_RAW);
        break;
    }
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

    int oldHP = getModifiedAttribute(ATTR_HP);
    int newHP = oldHP;
    int maxHP = getModifiedAttribute(ATTR_MAX_HP);

    // Regenerate HP
    if (mAction != DEAD && !isTimerRunning(T_B_HP_REGEN))
    {
        setTimerHard(T_B_HP_REGEN, TICKS_PER_HP_REGENERATION);
        newHP += getModifiedAttribute(ATTR_HP_REGEN);
    }
    // Cap HP at maximum
    if (newHP > maxHP)
    {
        newHP = maxHP;
    }
    // Only update HP when it actually changed to avoid network noise
    if (newHP != oldHP)
    {
        setAttribute(ATTR_HP, newHP);
        raiseUpdateFlags(UPDATEFLAG_HEALTHCHANGE);
    }

    // Update lifetime of effects.
    for (AttributeMap::iterator it = mAttributes.begin();
         it != mAttributes.end();
         ++it)
        if (it->second.tick())
            updateDerivedAttributes(it->first);

    // Update and run status effects
    StatusEffects::iterator it = mStatus.begin();
    while (it != mStatus.end())
    {
        it->second.time--;
        if (it->second.time > 0 && mAction != DEAD)
            it->second.status->tick(this, it->second.time);

        if (it->second.time <= 0 || mAction == DEAD)
        {
            mStatus.erase(it);
            it = mStatus.begin();
        }
        it++;
    }

    // Check if being died
    if (getModifiedAttribute(ATTR_HP) <= 0 && mAction != DEAD)
        died();
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

