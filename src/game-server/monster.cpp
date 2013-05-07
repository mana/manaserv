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

    AbilityComponent *abilityComponent = new AbilityComponent(entity);
    entity.addComponent(abilityComponent);
    for (auto *abilitiyInfo : specy->getAbilities())
    {
        abilityComponent->giveAbility(abilitiyInfo);
    }

    beingComponent->signal_died.connect(sigc::mem_fun(this,
                                            &MonsterComponent::monsterDied));
}

MonsterComponent::~MonsterComponent()
{
}

void MonsterComponent::update(Entity &entity)
{
    if (mKillStealProtectedTimeout.justFinished())
        mOwner = nullptr;

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
void MonsterComponent::monsterDied(Entity *monster)
{
    mDecayTimeout.set(DECAY_TIME);
}

