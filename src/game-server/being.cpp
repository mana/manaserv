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
 */

#include <cassert>

#include "game-server/being.hpp"

#include "defines.h"
#include "game-server/attackzone.hpp"
#include "game-server/collisiondetection.hpp"
#include "game-server/eventlistener.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/effect.hpp"
#include "utils/logger.h"

Being::Being(ThingType type):
    Actor(type),
    mAction(STAND),
    mSpeed(0),
    mDirection(0),
    mHpRegenTimer(0)
{
    Attribute attr = { 0, 0 };
    mAttributes.resize(NB_BEING_ATTRIBUTES, attr);
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

    if (HPloss < 0) HPloss = 0;

    mHitsTaken.push_back(HPloss);
    Attribute &HP = mAttributes[BASE_ATTR_HP];
    LOG_DEBUG("Being " << getPublicID() << " suffered "<<HPloss<<" damage. HP: "<<HP.base + HP.mod<<"/"<<HP.base);
    HP.mod -= HPloss;
    if (HPloss != 0) modifiedAttribute(BASE_ATTR_HP);

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

void Being::move()
{
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
    for (std::list<PATH_NODE>::iterator pathIterator = mPath.begin();
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
        mPath = map->findPath(tileSX, tileSY, tileDX, tileDY, getWalkMask());
    }

    if (mPath.empty())
    {
        // no path was found
        mDst = mOld;
        mActionTime = 0;
        return;
    }

    PATH_NODE prev(tileSX, tileSY);
    Point pos;
    do
    {
        PATH_NODE next = mPath.front();
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

void Being::performAttack(const Damage &damage, const AttackZone *attackZone)
{
    Point ppos = getPosition();
    const int attackAngle = directionToAngle(getDirection());

    std::list<Being *> victims;

    LOG_DEBUG("Direction:"<<getDirection()<<
              " range:"<<attackZone->range<<
              " angle:"<<attackZone->angle);

    Point attPos, attSize, defPos, defSize;
    if (attackZone->shape == ATTZONESHAPE_RECT)
    {
        if (getDirection() == DIRECTION_UP)
        {
            attPos.x = ppos.x - attackZone->angle;
            attPos.y = ppos.y - attackZone->range;
            attSize.x = attackZone->angle * 2;
            attSize.y = attackZone->range;
        }
        if (getDirection() == DIRECTION_DOWN)
        {
            attPos.x = ppos.x - attackZone->angle;
            attPos.y = ppos.y;
            attSize.x = attackZone->angle * 2;
            attSize.y = attackZone->range;
        }
        if (getDirection() == DIRECTION_RIGHT)
        {
            attPos.x = ppos.x;
            attPos.y = ppos.y - attackZone->angle;
            attSize.x = attackZone->range;
            attSize.y = attackZone->angle * 2;
        }
        if (getDirection() == DIRECTION_LEFT)
        {
            attPos.x = ppos.x - attackZone->range;
            attPos.y = ppos.y - attackZone->angle;
            attSize.x = attackZone->range;
            attSize.y = attackZone->angle * 2;
        }
        /* debug effect to see when and where the server pictures the attack - should
         * be moved to the client side when the attack detection works statisfactory.
         */
        Effects::show(26, getMap(), Point(attPos.x + attSize.x / 2, attPos.y + attSize.y / 2));
    }

    for (BeingIterator
         i(getMap()->getAroundActorIterator(this, attackZone->range)); i; ++i)
    {
        Being *b = *i;

        if (b == this)
            continue;

        const ThingType type = b->getType();
        if (type != OBJECT_CHARACTER && type != OBJECT_MONSTER)
            continue;

        if (getMap()->getPvP() == PVP_NONE &&
            type == OBJECT_CHARACTER &&
            getType() == OBJECT_CHARACTER)
            continue;

        LOG_DEBUG("Attack Zone:" << attPos.x << ":" << attPos.y <<
                  " " << attSize.x << "x" << attSize.y);
        LOG_DEBUG("Defender Zone:" << defPos.x << ":" << defPos.y <<
                  " " << defSize.x << "x" << defSize.y);

        const Point &opos = b->getPosition();

        switch (attackZone->shape)
        {
            case ATTZONESHAPE_CONE:
                if  (Collision::diskWithCircleSector(
                        opos, b->getSize(),
                        ppos, attackZone->range,
                        attackZone->angle / 2, attackAngle)
                    )
                {
                    victims.push_back(b);
                }
                break;
            case ATTZONESHAPE_RECT:
                defPos.x = opos.x - b->getSize();
                defPos.y = opos.y - b->getSize();
                defSize.x = b->getSize() * 2;
                defSize.y = b->getSize() * 2;
                if (Collision::rectWithRect(attPos, attSize, defPos, defSize))
                {
                    victims.push_back(b);
                }
                break;
            default:
                break;
        }
    }

    if (attackZone->multiTarget)
    {
        // damage everyone
        for (std::list<Being *>::iterator i = victims.begin();
             i != victims.end();
             i++)
        {
            (*i)->damage(this, damage);
        }
    }
    else
    {
        // find the closest and damage this one
        Being* closestVictim = NULL;
        int closestDistance = INT_MAX;
        for (std::list<Being *>::iterator i = victims.begin();
             i != victims.end();
             i++)
        {
            Point opos = (*i)->getPosition();
            int distance = abs(opos.x - ppos.x) + abs(opos.y - ppos.y);
            /* not using pythagoras here is a) faster and b) results in more natural
               target selection because targets closer to the center line of the
               attack angle are prioritized
            */
            if (distance < closestDistance)
            {
                closestVictim = (*i);
                closestDistance = distance;
            }
        }
        if (closestVictim) closestVictim->damage(this, damage);
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

void Being::update()
{
    int oldHP = getModifiedAttribute(BASE_ATTR_HP);
    int newHP = oldHP;
    int maxHP = getAttribute(BASE_ATTR_HP);

    // Regenerate HP
    if (mAction != DEAD && ++mHpRegenTimer >= TICKS_PER_HP_REGENERATION)
    {
        mHpRegenTimer = 0;
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

    // Check if being died
    if (getModifiedAttribute(BASE_ATTR_HP) <= 0 && mAction != DEAD)
    {
        died();
    }
}
