/*
 *  The Mana Server
 *  Copyright (C) 2004-2011  The Mana World Development Team
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

#include "game-server/monster.h"

#include "common/configuration.h"
#include "common/resourcemanager.h"
#include "game-server/attributemanager.h"
#include "game-server/character.h"
#include "game-server/collisiondetection.h"
#include "game-server/item.h"
#include "game-server/map.h"
#include "game-server/mapcomposite.h"
#include "game-server/state.h"
#include "scripting/scriptmanager.h"
#include "utils/logger.h"
#include "utils/speedconv.h"

#include <cmath>

struct MonsterTargetEventDispatch: EventDispatch
{
    MonsterTargetEventDispatch()
    {
        typedef EventListenerFactory<Monster, &Monster::mTargetListener> Factory;
        removed = &Factory::create< Entity, &Monster::forgetTarget >::function;
        died = &Factory::create<Entity, &Monster::forgetTarget, Being>::function;
    }
};

static MonsterTargetEventDispatch monsterTargetEventDispatch;

MonsterClass::~MonsterClass()
{
    for (std::vector<AttackInfo *>::iterator it = mAttacks.begin(),
         it_end = mAttacks.end(); it != it_end; ++it)
    {
        delete *it;
    }
}

Monster::Monster(MonsterClass *specy):
    Being(OBJECT_MONSTER),
    mSpecy(specy),
    mTargetListener(&monsterTargetEventDispatch),
    mOwner(NULL)
{
    LOG_DEBUG("Monster spawned! (id: " << mSpecy->getId() << ").");

    setWalkMask(Map::BLOCKMASK_WALL | Map::BLOCKMASK_CHARACTER);

    /*
     * Initialise the attribute structures.
     */
    const AttributeManager::AttributeScope &mobAttr =
        attributeManager->getAttributeScope(MonsterScope);

    for (AttributeManager::AttributeScope::const_iterator it = mobAttr.begin(),
         it_end = mobAttr.end(); it != it_end; ++it)
    {
        mAttributes.insert(std::pair< unsigned int, Attribute >
                           (it->first, Attribute(*it->second)));
    }

    /*
     * Set the attributes to the values defined by the associated monster
     * class with or without mutations as needed.
     */

    int mutation = specy->getMutation();

    for (AttributeMap::iterator it2 = mAttributes.begin(),
         it2_end = mAttributes.end(); it2 != it2_end; ++it2)
    {
        double attr = 0.0f;

        if (specy->hasAttribute(it2->first))
        {
            attr = specy->getAttribute(it2->first);

            setAttribute(it2->first,
                  mutation ?
                  attr * (100 + (rand() % (mutation * 2)) - mutation) / 100.0 :
                  attr);
        }
    }

    mDamageMutation = mutation ?
                (100 + (rand() % (mutation * 2)) - mutation) / 100.0 : 1;

    setSize(specy->getSize());
    setGender(specy->getGender());

    // Set positions relative to target from which the monster can attack
    int dist = specy->getAttackDistance();
    mAttackPositions.push_back(AttackPosition(dist, 0, LEFT));
    mAttackPositions.push_back(AttackPosition(-dist, 0, RIGHT));
    mAttackPositions.push_back(AttackPosition(0, -dist, DOWN));
    mAttackPositions.push_back(AttackPosition(0, dist, UP));

    // Take attacks from specy
    std::vector<AttackInfo *> &attacks = specy->getAttackInfos();
    for (std::vector<AttackInfo *>::iterator it = attacks.begin(),
         it_end = attacks.end(); it != it_end; ++it)
    {
        addAttack(*it);
    }

    // Load default script
    loadScript(specy->getScript());
}

Monster::~Monster()
{
    // Remove death listeners.
    for (std::map<Being *, int>::iterator i = mAnger.begin(),
         i_end = mAnger.end(); i != i_end; ++i)
    {
        i->first->removeListener(&mTargetListener);
    }
}

void Monster::update()
{
    Being::update();

    if (mKillStealProtectedTimeout.justFinished())
        mOwner = NULL;

    // If dead, remove it
    if (mAction == DEAD)
    {
        if (mDecayTimeout.expired())
            GameState::enqueueRemove(this);

        return;
    }

    if (mSpecy->getUpdateCallback().isValid())
    {
        Script *script = ScriptManager::currentState();
        script->setMap(getMap());
        script->prepare(mSpecy->getUpdateCallback());
        script->push(this);
        script->execute();
    }

    refreshTarget();

    // Cancel the rest when we have a target
    if (mTarget)
        return;

    // We have no target - let's wander around
    if (mStrollTimeout.expired() && getPosition() == getDestination())
    {
        if (mKillStealProtectedTimeout.expired())
        {
            unsigned range = mSpecy->getStrollRange();
            if (range)
            {
                Point randomPos(rand() % (range * 2 + 1)
                                - range + getPosition().x,
                                rand() % (range * 2 + 1)
                                - range + getPosition().y);
                // Don't allow negative destinations, to avoid rounding
                // problems when divided by tile size
                if (randomPos.x >= 0 && randomPos.y >= 0)
                    setDestination(randomPos);
            }
            mStrollTimeout.set(10 + rand() % 10);
        }
    }
}

void Monster::refreshTarget()
{
    // We are dead and sadly not possible to keep attacking :(
    if (mAction == DEAD)
        return;

    // Check potential attack positions
    int bestTargetPriority = 0;
    Being *bestTarget = 0;
    Point bestAttackPosition;

    // reset Target. We will find a new one if possible
    mTarget = 0;

    // Iterate through objects nearby
    int aroundArea = Configuration::getValue("game_visualRange", 448);
    for (BeingIterator i(getMap()->getAroundBeingIterator(this, aroundArea));
         i; ++i)
    {
        // We only want to attack player characters
        if ((*i)->getType() != OBJECT_CHARACTER)
            continue;

        Being *target = static_cast<Being *>(*i);

        // Dead characters are ignored
        if (target->getAction() == DEAD)
            continue;

        // Determine how much we hate the target
        int targetPriority = 0;
        std::map<Being *, int, std::greater<Being *> >::iterator angerIterator;
        angerIterator = mAnger.find(target);
        if (angerIterator != mAnger.end())
        {
            targetPriority = angerIterator->second;
        }
        else if (mSpecy->isAggressive())
        {
            targetPriority = 1;
        }
        else
        {
            continue;
        }

        // Check all attack positions
        for (std::list<AttackPosition>::iterator j = mAttackPositions.begin();
             j != mAttackPositions.end(); j++)
        {
            Point attackPosition = target->getPosition();
            attackPosition.x += j->x;
            attackPosition.y += j->y;

            int posPriority = calculatePositionPriority(attackPosition,
                                                        targetPriority);
            if (posPriority > bestTargetPriority)
            {
                bestTargetPriority = posPriority;
                bestTarget = target;
                bestAttackPosition = attackPosition;
            }
        }
    }
    if (bestTarget)
    {
        mTarget = bestTarget;
        if (bestAttackPosition == getPosition())
        {
            mAction = ATTACK;
            updateDirection(getPosition(), mTarget->getPosition());
        }
        else
        {
            setDestination(bestAttackPosition);
        }
    }
}

void Monster::processAttack(Attack &attack)
{
    if (!mTarget)
    {
        setAction(STAND);
        return;
    }

    Damage dmg = attack.getAttackInfo()->getDamage();
    dmg.skill   = 0;
    dmg.base    *= mDamageMutation;
    dmg.delta   *= mDamageMutation;

    int hit = performAttack(mTarget, attack.getAttackInfo()->getDamage());

    const Script::Ref &scriptCallback =
            attack.getAttackInfo()->getScriptCallback();

    if (scriptCallback.isValid() && hit > -1)
    {
        Script *script = ScriptManager::currentState();
        script->setMap(getMap());
        script->prepare(scriptCallback);
        script->push(this);
        script->push(mTarget);
        script->push(hit);
        script->execute();
    }
}

void Monster::loadScript(const std::string &scriptName)
{
    if (scriptName.length() == 0)
        return;

    std::stringstream filename;
    filename << "scripts/monster/" << scriptName;
    if (ResourceManager::exists(filename.str()))
    {
        LOG_INFO("Loading monster script: " << filename.str());
        ScriptManager::currentState()->loadFile(filename.str());
    }
    else
    {
        LOG_WARN("Could not find script file \""
                 << filename.str() << "\" for monster");
    }
}

int Monster::calculatePositionPriority(Point position, int targetPriority)
{
    Point thisPos = getPosition();

    unsigned range = mSpecy->getTrackRange();

    Map *map = getMap()->getMap();
    int tileWidth = map->getTileWidth();
    int tileHeight = map->getTileHeight();

    // Check if we already are on this position
    if (thisPos.x / tileWidth == position.x / tileWidth &&
        thisPos.y / tileHeight == position.y / tileHeight)
    {
        return targetPriority *= range;
    }

    Path path;
    path = map->findPath(thisPos.x / tileWidth, thisPos.y / tileHeight,
                         position.x / tileWidth, position.y / tileHeight,
                         getWalkMask(),
                         range);

    if (path.empty() || path.size() >= range)
    {
        return 0;
    }
    else
    {
        return targetPriority * (range - path.size());
    }
}

void Monster::forgetTarget(Entity *t)
{
    Being *b = static_cast< Being * >(t);
    mAnger.erase(b);
    b->removeListener(&mTargetListener);

    if (b->getType() == OBJECT_CHARACTER)
    {
        Character *c = static_cast< Character * >(b);
        mExpReceivers.erase(c);
        mLegalExpReceivers.erase(c);
    }
}

void Monster::changeAnger(Actor *target, int amount)
{
    if (target && (target->getType() == OBJECT_MONSTER
        || target->getType() == OBJECT_CHARACTER))
    {
        Being *t = static_cast< Being * >(target);
        if (mAnger.find(t) != mAnger.end())
        {
            mAnger[t] += amount;
        }
        else
        {
            mAnger[t] = amount;
            t->addListener(&mTargetListener);
        }
    }
}

int Monster::damage(Actor *source, const Damage &damage)
{
    int HPLoss = Being::damage(source, damage);
    if (source)
    {
        changeAnger(source, HPLoss);
    }

    if (HPLoss && source && source->getType() == OBJECT_CHARACTER)
    {
        Character *s = static_cast< Character * >(source);

        mExpReceivers[s].insert(damage.skill);
        if (mKillStealProtectedTimeout.expired() || mOwner == s
            || mOwner->getParty() == s->getParty())
        {
            mOwner = s;
            mLegalExpReceivers.insert(s);
            mKillStealProtectedTimeout.set(KILLSTEAL_PROTECTION_TIME);
        }
    }

    if (mSpecy->getDamageCallback().isValid())
    {
        Script *script = ScriptManager::currentState();
        script->setMap(getMap());
        script->prepare(mSpecy->getDamageCallback());
        script->push(this);
        script->push(source);
        script->push(HPLoss);
        // TODO: add exact damage parameters as well
        script->execute();
    }

    return HPLoss;
}

void Monster::died()
{
    if (mAction == DEAD) return;

    Being::died();
    mDecayTimeout.set(DECAY_TIME);

    if (mExpReceivers.size() > 0)
    {
        // If the monster was killed by players, randomly drop items.
        const unsigned size = mSpecy->mDrops.size();
        for (unsigned i = 0; i < size; i++)
        {
            const int p = rand() / (RAND_MAX / 10000);
            if (p <= mSpecy->mDrops[i].probability)
            {
                Item *item = new Item(mSpecy->mDrops[i].item, 1);
                item->setMap(getMap());
                item->setPosition(getPosition());
                GameState::enqueueInsert(item);
            }
        }

        // Distribute exp reward.
        std::map<Character *, std::set <size_t> > ::iterator iChar;
        std::set<size_t>::iterator iSkill;


        float expPerChar = (float)mSpecy->getExp() / mExpReceivers.size();

        for (iChar = mExpReceivers.begin(); iChar != mExpReceivers.end();
             iChar++)
        {
            Character *character = (*iChar).first;
            std::set<size_t> *skillSet = &(*iChar).second;

            if (mLegalExpReceivers.find(character) == mLegalExpReceivers.end()
                || skillSet->size() < 1)
                continue;

            int expPerSkill = int(expPerChar / skillSet->size());
            for (iSkill = skillSet->begin(); iSkill != skillSet->end();
                 iSkill++)
            {
                character->receiveExperience(*iSkill, expPerSkill,
                                             mSpecy->getOptimalLevel());
            }
            character->incrementKillCount(mSpecy->getId());
        }
    }
}

bool Monster::recalculateBaseAttribute(unsigned int attr)
{
    LOG_DEBUG("Monster: Received update attribute recalculation request for "
              << attr << ".");
    if (!mAttributes.count(attr))
    {
        LOG_DEBUG("Monster::recalculateBaseAttribute: "
                  << attr << " not found!");
        return false;
    }
    double newBase = getAttribute(attr);

    switch (attr)
    {
      // Those a set only at load time.
      case ATTR_MAX_HP:
      case ATTR_DODGE:
      case ATTR_MAGIC_DODGE:
      case ATTR_ACCURACY:
      case ATTR_DEFENSE:
      case ATTR_MAGIC_DEFENSE:
      case ATTR_HP_REGEN:
      case ATTR_MOVE_SPEED_TPS:
      case ATTR_INV_CAPACITY:
          // nothing to do.
          break;

      // Only HP and Speed Raw updated for monsters
      default:
          Being::recalculateBaseAttribute(attr);
          break;
    }
    if (newBase != getAttribute(attr))
    {
        setAttribute(attr, newBase);
        return true;
    }
    LOG_DEBUG("Monster: No changes to sync for attribute '" << attr << "'.");
    return false;
}
