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

MonsterComponent::MonsterComponent(Entity &entity, MonsterClass *specy):
    mSpecy(specy)
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
void MonsterComponent::update(Entity &entity)
{
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
}

void MonsterComponent::monsterDied(Entity *monster)
{
    mDecayTimeout.set(DECAY_TIME);
}

