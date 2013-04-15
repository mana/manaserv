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
#include "game-server/monstercombatcomponent.h"
#include "game-server/state.h"
#include "scripting/scriptmanager.h"
#include "utils/logger.h"
#include "utils/speedconv.h"

#include <cmath>

MonsterClass::~MonsterClass()
{
    for (std::vector<AttackInfo *>::iterator it = mAttacks.begin(),
         it_end = mAttacks.end(); it != it_end; ++it)
    {
        delete *it;
    }
}

double MonsterClass::getVulnerability(Element element) const
{
    Vulnerabilities::const_iterator it = mVulnerabilities.find(element);
    if (it == mVulnerabilities.end())
        return 1.0f;
    return it->second;
}

MonsterComponent::MonsterComponent(Entity &entity, MonsterClass *specy):
    mSpecy(specy),
    mOwner(nullptr)
{
    LOG_DEBUG("Monster spawned! (id: " << mSpecy->getId() << ").");

    auto *beingComponent = entity.getComponent<BeingComponent>();

    auto *actorComponent = entity.getComponent<ActorComponent>();
    actorComponent->setWalkMask(Map::BLOCKMASK_WALL |
                                Map::BLOCKMASK_CHARACTER);
    actorComponent->setBlockType(BLOCKTYPE_MONSTER);
    actorComponent->setSize(specy->getSize());

    /*
     * Initialise the attribute structures.
     */

    for (auto attrInfo : attributeManager->getAttributeScope(MonsterScope))
    {
        beingComponent->createAttribute(attrInfo.first, *attrInfo.second);
    }

    /*
     * Set the attributes to the values defined by the associated monster
     * class with or without mutations as needed.
     */

    int mutation = specy->getMutation();

    for (auto &attribute : specy->getAttributes())
    {
        double attributeValue = attribute.second;
        if (mutation != 0)
        {
            double factor = 100 + (rand() % (mutation * 2)) - mutation;
            attributeValue = attributeValue * factor / 100.0;
        }
        beingComponent->setAttribute(entity, attribute.first, attributeValue);
    }

    beingComponent->setGender(specy->getGender());

    beingComponent->signal_died.connect(sigc::mem_fun(this,
                                            &MonsterComponent::monsterDied));

    // Set positions relative to target from which the monster can attack
    int dist = specy->getAttackDistance();
    mAttackPositions.push_back(AttackPosition(dist, 0, LEFT));
    mAttackPositions.push_back(AttackPosition(-dist, 0, RIGHT));
    mAttackPositions.push_back(AttackPosition(0, -dist, DOWN));
    mAttackPositions.push_back(AttackPosition(0, dist, UP));

    MonsterCombatComponent *combatComponent =
            new MonsterCombatComponent(entity, specy);
    entity.addComponent(combatComponent);

    double damageMutation = mutation ?
                (100.0 + (rand() % (mutation * 2)) - mutation) / 100.0 : 1.0;
    combatComponent->setDamageMutation(damageMutation);

    combatComponent->signal_damaged.connect(
            sigc::mem_fun(this, &MonsterComponent::receivedDamage));
}

MonsterComponent::~MonsterComponent()
{
}

void MonsterComponent::update(Entity &entity)
{
    if (mKillStealProtectedTimeout.justFinished())
        mOwner = NULL;

    auto *beingComponent = entity.getComponent<BeingComponent>();

    // If dead, remove it
    if (beingComponent->getAction() == DEAD)
    {
        if (mDecayTimeout.expired())
            GameState::enqueueRemove(&entity);

        return;
    }

    if (mSpecy->getUpdateCallback().isValid())
    {
        Script *script = ScriptManager::currentState();
        script->prepare(mSpecy->getUpdateCallback());
        script->push(&entity);
        script->execute(entity.getMap());
    }

    refreshTarget(entity);

    // Cancel the rest when we have a target
    if (entity.getComponent<CombatComponent>()->getTarget())
        return;

    const Point &position =
            entity.getComponent<ActorComponent>()->getPosition();

    // We have no target - let's wander around
    if (mStrollTimeout.expired() &&
            position == beingComponent->getDestination())
    {
        if (mKillStealProtectedTimeout.expired())
        {
            unsigned range = mSpecy->getStrollRange();
            if (range)
            {
                Point randomPos(rand() % (range * 2 + 1) - range + position.x,
                                rand() % (range * 2 + 1) - range + position.y);
                // Don't allow negative destinations, to avoid rounding
                // problems when divided by tile size
                if (randomPos.x >= 0 && randomPos.y >= 0)
                    beingComponent->setDestination(entity, randomPos);
            }
            mStrollTimeout.set(10 + rand() % 10);
        }
    }
}

void MonsterComponent::refreshTarget(Entity &entity)
{
    auto *beingComponent = entity.getComponent<BeingComponent>();

    // We are dead and sadly not possible to keep attacking :(
    if (beingComponent->getAction() == DEAD)
        return;

    // Check potential attack positions
    int bestTargetPriority = 0;
    Entity *bestTarget = 0;
    Point bestAttackPosition;

    // reset Target. We will find a new one if possible
    entity.getComponent<CombatComponent>()->clearTarget();

    // Iterate through objects nearby
    int aroundArea = Configuration::getValue("game_visualRange", 448);
    for (BeingIterator i(entity.getMap()->getAroundBeingIterator(&entity,
                                                                 aroundArea));
         i; ++i)
    {
        // We only want to attack player characters
        if ((*i)->getType() != OBJECT_CHARACTER)
            continue;

        Entity *target = *i;

        // Dead characters are ignored
        if (beingComponent->getAction() == DEAD)
            continue;

        // Determine how much we hate the target
        int targetPriority = 0;
        std::map<Entity *, AggressionInfo>::iterator angerIterator =
                mAnger.find(target);
        if (angerIterator != mAnger.end())
        {
            const AggressionInfo &aggressionInfo = angerIterator->second;
            targetPriority = aggressionInfo.anger;
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
            Point attackPosition =
                    target->getComponent<ActorComponent>()->getPosition();
            attackPosition.x += j->x;
            attackPosition.y += j->y;

            int posPriority = calculatePositionPriority(entity,
                                                        attackPosition,
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
        const Point &ownPosition =
                entity.getComponent<ActorComponent>()->getPosition();
        const Point &targetPosition =
                bestTarget->getComponent<ActorComponent>()->getPosition();

        entity.getComponent<CombatComponent>()->setTarget(bestTarget);
        if (bestAttackPosition == ownPosition)
        {
            beingComponent->setAction(entity, ATTACK);
            beingComponent->updateDirection(entity, ownPosition,
                                            targetPosition);
        }
        else
        {
            beingComponent->setDestination(entity, bestAttackPosition);
        }
    }
}

int MonsterComponent::calculatePositionPriority(Entity &entity,
                                                Point position,
                                                int targetPriority)
{
    Point thisPos = entity.getComponent<ActorComponent>()->getPosition();

    unsigned range = mSpecy->getTrackRange();

    Map *map = entity.getMap()->getMap();
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
                         entity.getComponent<ActorComponent>()->getWalkMask(),
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

void MonsterComponent::forgetTarget(Entity *entity)
{
    {
        AggressionInfo &aggressionInfo = mAnger[entity];
        aggressionInfo.removedConnection.disconnect();
        aggressionInfo.diedConnection.disconnect();
    }
    mAnger.erase(entity);

    if (entity->getType() == OBJECT_CHARACTER)
    {
        mExpReceivers.erase(entity);
        mLegalExpReceivers.erase(entity);
    }
}

void MonsterComponent::changeAnger(Entity *target, int amount)
{
    const EntityType type = target->getType();
    if (type != OBJECT_MONSTER && type != OBJECT_CHARACTER)
        return;

    if (mAnger.find(target) != mAnger.end())
    {
        mAnger[target].anger += amount;
    }
    else
    {
        AggressionInfo &aggressionInfo = mAnger[target];
        aggressionInfo.anger = amount;

        // Forget target either when it's removed or died, whichever
        // happens first.
        aggressionInfo.removedConnection =
                target->signal_removed.connect(sigc::mem_fun(this, &MonsterComponent::forgetTarget));
        aggressionInfo.diedConnection = target->getComponent<BeingComponent>()
                ->signal_died.connect(
                        sigc::mem_fun(this, &MonsterComponent::forgetTarget));
    }
}

std::map<Entity *, int> MonsterComponent::getAngerList() const
{
    std::map<Entity *, int> result;
    std::map<Entity *, AggressionInfo>::const_iterator i, i_end;

    for (i = mAnger.begin(), i_end = mAnger.end(); i != i_end; ++i)
    {
        const AggressionInfo &aggressionInfo = i->second;
        result.insert(std::make_pair(i->first, aggressionInfo.anger));
    }

    return result;
}

void MonsterComponent::monsterDied(Entity *monster)
{
    mDecayTimeout.set(DECAY_TIME);

    if (mExpReceivers.size() > 0)
    {
        // If the monster was killed by players, randomly drop items.
        const unsigned size = mSpecy->mDrops.size();
        for (unsigned i = 0; i < size; i++)
        {
            const int p = rand() / (RAND_MAX / 10000);
            const MonsterDrop &drop = mSpecy->mDrops[i];

            if (p <= drop.probability)
            {
                const Point &position =
                        monster->getComponent<ActorComponent>()->getPosition();
                Entity *item = Item::create(monster->getMap(), position,
                                            drop.item, 1);
                GameState::enqueueInsert(item);
            }
        }

        // Distribute exp reward.
        std::map<Entity *, std::set <size_t> > ::iterator iChar;
        std::set<size_t>::iterator iSkill;


        float expPerChar = (float)mSpecy->getExp() / mExpReceivers.size();

        for (iChar = mExpReceivers.begin(); iChar != mExpReceivers.end();
             iChar++)
        {
            auto *character = (*iChar).first;
            const std::set<size_t> &skillSet = (*iChar).second;

            if (mLegalExpReceivers.find(character) == mLegalExpReceivers.end()
                || skillSet.empty())
                continue;

            auto characterComponent =
                    character->getComponent<CharacterComponent>();

            int expPerSkill = int(expPerChar / skillSet.size());
            for (iSkill = skillSet.begin(); iSkill != skillSet.end();
                 iSkill++)
            {
                characterComponent->receiveExperience(*iSkill, expPerSkill,
                                                    mSpecy->getOptimalLevel());
            }
            characterComponent->incrementKillCount(mSpecy->getId());
        }
    }
}


void MonsterComponent::receivedDamage(Entity *source, const Damage &damage, int hpLoss)
{
    if (source)
        changeAnger(source, hpLoss);

    if (hpLoss && source && source->getType() == OBJECT_CHARACTER)
    {
        mExpReceivers[source].insert(damage.skill);
        if (mKillStealProtectedTimeout.expired() || mOwner == source
            || mOwner->getComponent<CharacterComponent>()->getParty() ==
                    source->getComponent<CharacterComponent>()->getParty())
        {
            mOwner = source;
            mLegalExpReceivers.insert(source);
            mKillStealProtectedTimeout.set(KILLSTEAL_PROTECTION_TIME);
        }
    }
}
