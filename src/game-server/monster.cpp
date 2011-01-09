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

#include "game-server/monster.h"

#include "common/configuration.h"
#include "common/resourcemanager.h"
#include "game-server/attributemanager.h"
#include "game-server/character.h"
#include "game-server/collisiondetection.h"
#include "game-server/item.h"
#include "game-server/mapcomposite.h"
#include "game-server/state.h"
#include "scripting/script.h"
#include "utils/logger.h"
#include "utils/speedconv.h"

#include <cmath>

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

struct MonsterTargetEventDispatch: EventDispatch
{
    MonsterTargetEventDispatch()
    {
        typedef EventListenerFactory<Monster, &Monster::mTargetListener> Factory;
        removed = &Factory::create< Thing, &Monster::forgetTarget >::function;
        died = &Factory::create<Thing, &Monster::forgetTarget, Being>::function;
    }
};

static MonsterTargetEventDispatch monsterTargetEventDispatch;

Monster::Monster(MonsterClass *specy):
    Being(OBJECT_MONSTER),
    mSpecy(specy),
    mScript(NULL),
    mTargetListener(&monsterTargetEventDispatch),
    mOwner(NULL),
    mCurrentAttack(NULL)
{
    LOG_DEBUG("Monster spawned! (id: " << mSpecy->getId() << ").");

    /*
     * Initialise the attribute structures.
     */
    const AttributeScopes &mobAttr = attributeManager->getAttributeInfoForType(
                                                                      ATTR_MOB);

    for (AttributeScopes::const_iterator it = mobAttr.begin(),
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
            attr = specy->getAttribute(it2->first);

        setAttribute(it2->first,
                  mutation ?
                  attr * (100 + (rand()%(mutation << 1)) - mutation) / 100.0 :
                  attr);
    }

    // Set the speed in tiles per second.
    setAttribute(ATTR_MOVE_SPEED_RAW,
                 utils::tpsToRawSpeed(getAttribute(ATTR_MOVE_SPEED_TPS)));
    setSize(specy->getSize());

    // Set positions relative to target from which the monster can attack
    int dist = specy->getAttackDistance();
    mAttackPositions.push_back(AttackPosition(dist, 0, LEFT));
    mAttackPositions.push_back(AttackPosition(-dist, 0, RIGHT));
    mAttackPositions.push_back(AttackPosition(0, -dist, DOWN));
    mAttackPositions.push_back(AttackPosition(0, dist, UP));

    // Load default script
    loadScript(specy->getScript());
}

Monster::~Monster()
{
    // Remove the monster's script if it has one
    if (mScript)
        delete mScript;

    // Remove death listeners.
    for (std::map<Being *, int>::iterator i = mAnger.begin(),
         i_end = mAnger.end(); i != i_end; ++i)
    {
        i->first->removeListener(&mTargetListener);
    }

    // Free the map position
    if (getMap())
    {
        Point oldP = getPosition();
        Map *map = getMap()->getMap();
        int tileWidth = map->getTileWidth();
        int tileHeight = map->getTileHeight();
        map->freeTile(oldP.x / tileWidth, oldP.y / tileHeight, getBlockType());
    }
}

void Monster::perform()
{
    if (mAction == ATTACK)
    {
        if (mTarget)
        {
            if (mCurrentAttack)
            {
                if (!isTimerRunning(T_M_ATTACK_TIME))
                {
                    setTimerHard(T_M_ATTACK_TIME, mCurrentAttack->aftDelay
                                                  + mCurrentAttack->preDelay);
                    Damage dmg(getModifiedAttribute(MOB_ATTR_PHY_ATK_MIN) *
                                    mCurrentAttack->damageFactor,
                               getModifiedAttribute(MOB_ATTR_PHY_ATK_DELTA) *
                                    mCurrentAttack->damageFactor,
                               getModifiedAttribute(ATTR_ACCURACY),
                               mCurrentAttack->element,
                               mCurrentAttack->type,
                               mCurrentAttack->range);

                    int hit = performAttack(mTarget, mCurrentAttack->range,
                                            dmg);

                    if (! mCurrentAttack->scriptFunction.empty()
                        && mScript
                        && hit > -1)
                    {
                        mScript->setMap(getMap());
                        mScript->prepare(mCurrentAttack->scriptFunction);
                        mScript->push(this);
                        mScript->push(mTarget);
                        mScript->push(hit);
                        mScript->execute();
                    }
                }
            }
        }
        else
        {
            setAction(STAND);
        }
    }
}

void Monster::update()
{
    Being::update();

    if (isTimerJustFinished(T_M_KILLSTEAL_PROTECTED))
    {
        mOwner = NULL;
    }

    // If dead, remove it
    if (mAction == DEAD)
    {
        if (!isTimerRunning(T_M_DECAY))
        {
            GameState::enqueueRemove(this);
        }
        return;
    }
    else if(mScript)
    {
        mScript->setMap(getMap());
        mScript->prepare("update");
        mScript->push(this);
        mScript->execute();
    }

    // Cancel the rest when we are currently performing an attack
    if (isTimerRunning(T_M_ATTACK_TIME))
        return;

    // Check potential attack positions
    Being *bestAttackTarget = mTarget = NULL;
    int bestTargetPriority = 0;
    Point bestAttackPosition;
    BeingDirection bestAttackDirection = DOWN;

    // Iterate through objects nearby
    int aroundArea = Configuration::getValue("game_visualRange", 448);
    for (BeingIterator i(getMap()->getAroundBeingIterator(this, aroundArea));
         i; ++i)
    {
        // We only want to attack player characters
        if ((*i)->getType() != OBJECT_CHARACTER)
            continue;

        Being *target = static_cast<Being *> (*i);

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
            Point attackPosition = (*i)->getPosition();
            attackPosition.x += (*j).x;
            attackPosition.y += (*j).y;

            int posPriority = calculatePositionPriority(attackPosition,
                                                        targetPriority);
            if (posPriority > bestTargetPriority)
            {
                bestAttackTarget = mTarget = target;
                bestTargetPriority = posPriority;
                bestAttackPosition = attackPosition;
                bestAttackDirection = (*j).direction;
            }
        }
    }

    // Check if an enemy has been found
    if (bestAttackTarget)
    {
        // Check which attacks have a chance to hit the target
        MonsterAttacks allAttacks = mSpecy->getAttacks();
        std::map<int, MonsterAttack *> workingAttacks;
        int prioritySum = 0;

        for (MonsterAttacks::iterator i = allAttacks.begin();
             i != allAttacks.end();
             i++)
        {
            int distx = this->getPosition().x
                        - bestAttackTarget->getPosition().x;
            int disty = this->getPosition().y
                        - bestAttackTarget->getPosition().y;
            int distSquare = (distx * distx + disty * disty);
            int maxDist =  (*i)->range + bestAttackTarget->getSize();

            if (maxDist * maxDist >= distSquare)
            {
                prioritySum += (*i)->priority;
                workingAttacks[prioritySum] = (*i);
            }
        }

        if (workingAttacks.empty() || !prioritySum)
        {   //when no attack can hit move closer to attack position
            setDestination(bestAttackPosition);
        }
        else
        {
            // Prepare for using a random attack which can hit the enemy
            // Stop movement
            setDestination(getPosition());
            // Turn into direction of enemy
            setDirection(bestAttackDirection);
            // Perform a random attack based on priority
            mCurrentAttack =
                         workingAttacks.upper_bound(rand()%prioritySum)->second;
            setAction(ATTACK);
            raiseUpdateFlags(UPDATEFLAG_ATTACK);
        }
    }
    else
    {
        // We have no target - let's wander around
        if (getPosition() == getDestination())
        {
            if (!isTimerRunning(T_M_KILLSTEAL_PROTECTED))
            {
                unsigned range = mSpecy->getStrollRange();
                if (range)
                {
                    Point randomPos(rand() % (range * 2 + 1)
                                    - range + getPosition().x,
                                    rand() % (range * 2 + 1)
                                    - range + getPosition().y);
                    setDestination(randomPos);
                }
                setTimerHard(T_M_STROLL, 10 + rand() % 10);
            }
        }
    }
}

void Monster::loadScript(const std::string &scriptName)
{
    // A script may have already been loaded for this monster
    delete mScript;
    mScript = 0;

    if (scriptName.length() == 0)
        return;

    std::stringstream filename;
    filename << "scripts/monster/" << scriptName;
    if (ResourceManager::exists(filename.str()))
    {
        LOG_INFO("Loading monster script: " << filename.str());
        mScript = Script::create("lua");
        mScript->loadFile(filename.str());
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

void Monster::forgetTarget(Thing *t)
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

        std::list<size_t>::const_iterator iSkill;
        for (iSkill = damage.usedSkills.begin();
             iSkill != damage.usedSkills.end(); ++iSkill)
        {
            if (*iSkill)
            {
                mExpReceivers[s].insert(*iSkill);
                if (!isTimerRunning(T_M_KILLSTEAL_PROTECTED) || mOwner == s
                    || mOwner->getParty() == s->getParty())
                {
                    mOwner = s;
                    mLegalExpReceivers.insert(s);
                    setTimerHard(T_M_KILLSTEAL_PROTECTED,
                                 KILLSTEAL_PROTECTION_TIME);
                }
            }
        }
    }
    return HPLoss;
}

void Monster::died()
{
    if (mAction == DEAD) return;

    Being::died();
    setTimerHard(T_M_DECAY, Monster::DECAY_TIME);

    if (mExpReceivers.size() > 0)
    {
        // If the monster was killed by players, randomly drop an item.
        if (ItemClass *drop = mSpecy->getRandomDrop())
        {
            Item *item = new Item(drop, 1);
            item->setMap(getMap());
            item->setPosition(getPosition());
            GameState::enqueueInsert(item);
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

