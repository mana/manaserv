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


#include <string>
#include <map>

#include "game-server/item.h"

#include "common/configuration.h"
#include "game-server/attack.h"
#include "game-server/attributemanager.h"
#include "game-server/being.h"
#include "game-server/state.h"
#include "scripting/script.h"
#include "scripting/scriptmanager.h"

bool ItemEffectAttrMod::apply(Being *itemUser)
{
    LOG_DEBUG("Applying modifier.");
    itemUser->applyModifier(mAttributeId, mMod, mAttributeLayer,
                            mDuration, mId);
    return false;
}

void ItemEffectAttrMod::dispell(Being *itemUser)
{
    LOG_DEBUG("Dispelling modifier.");
    itemUser->removeModifier(mAttributeId, mMod, mAttributeLayer,
                             mId, !mDuration);
}

bool ItemEffectAttack::apply(Being * /* itemUser */)
{
    // TODO - STUB
    return false;
}

void ItemEffectAttack::dispell(Being * /* itemUser */)
{
    // TODO
}

ItemEffectScript::~ItemEffectScript()
{
}

bool ItemEffectScript::apply(Being *itemUser)
{
    if (mActivateEventName.empty())
        return false;

    Script::Ref function = mItemClass->getEventCallback(mActivateEventName);
    if (function.isValid())
    {
        Script *script = ScriptManager::currentState();
        script->setMap(itemUser->getMap());
        script->prepare(function);
        script->push(itemUser);
        script->push(mItemClass->getDatabaseID());
        script->execute(); // TODO return depending on script execution success.
        return true;
    }
    return false;
}

void ItemEffectScript::dispell(Being *itemUser)
{
    if (mDispellEventName.empty())
        return;

    Script::Ref function = mItemClass->getEventCallback(mDispellEventName);
    if (function.isValid())
    {
        Script *script = ScriptManager::currentState();
        script->setMap(itemUser->getMap());
        script->prepare(function);
        script->push(itemUser);
        script->push(mItemClass->getDatabaseID());
        script->execute();
    }
}

ItemClass::~ItemClass()
{
    while (mEffects.begin() != mEffects.end())
    {
        delete mEffects.begin()->second;
        mEffects.erase(mEffects.begin());
    }
}

void ItemClass::addEffect(ItemEffectInfo *effect,
                          ItemTriggerType id,
                          ItemTriggerType dispell)
{
    mEffects.insert(std::make_pair(id, effect));
    if (dispell)
        mDispells.insert(std::make_pair(dispell, effect));
}

bool ItemClass::useTrigger(Being *itemUser, ItemTriggerType trigger)
{
    if (!trigger)
        return false;

    std::pair<std::multimap< ItemTriggerType, ItemEffectInfo * >::iterator,
              std::multimap< ItemTriggerType, ItemEffectInfo * >::iterator>
      rn = mEffects.equal_range(trigger);
    bool ret = false;
    while (rn.first != rn.second)
        if (rn.first++->second->apply(itemUser))
            ret = true;

    rn = mDispells.equal_range(trigger);
    while (rn.first != rn.second)
        rn.first++->second->dispell(itemUser);

    return ret;
}


Item::Item(ItemClass *type, int amount)
          : Actor(OBJECT_ITEM), mType(type), mAmount(amount)
{
    mLifetime = Configuration::getValue("game_floorItemDecayTime", 0) * 10;
}

void Item::update()
{
    if (mLifetime)
    {
        mLifetime--;
        if (!mLifetime)
            GameState::enqueueRemove(this);
    }
}
