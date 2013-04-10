/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *  Copyright (C) 2012  The Mana Developers
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

#include "game-server/effect.h"

#include "game-server/being.h"
#include "game-server/entity.h"
#include "game-server/state.h"

const ComponentType EffectComponent::type;

void EffectComponent::update(Entity &entity)
{
    if (mHasBeenShown)
        GameState::enqueueRemove(static_cast<Actor*>(&entity));
}

namespace Effects
{
    void show(int id, MapComposite *map, const Point &pos)
    {
        Actor *effect = new Actor(OBJECT_EFFECT);
        effect->addComponent(new EffectComponent(id));
        effect->setMap(map);
        effect->setPosition(pos);

        GameState::enqueueInsert(effect);
    }

    void show(int id, Actor *b)
    {
        EffectComponent *effectComponent = new EffectComponent(id);
        effectComponent->setBeing(b);

        Actor *effect = new Actor(OBJECT_EFFECT);
        effect->addComponent(effectComponent);
        effect->setMap(b->getMap());
        effect->setPosition(b->getPosition());

        GameState::enqueueInsert(effect);
    }
}
