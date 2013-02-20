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

bool ItemEffectAttack::apply(Being *itemUser)
{
    itemUser->addAttack(mAttackInfo);
    return false;
}

void ItemEffectAttack::dispell(Being *itemUser)
{
    itemUser->removeAttack(mAttackInfo);
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
        script->prepare(function);
        script->push(itemUser);
        script->push(mItemClass->getDatabaseID());
        script->execute(itemUser->getMap());
        // TODO return depending on script execution success.
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
        script->prepare(function);
        script->push(itemUser);
        script->push(mItemClass->getDatabaseID());
        script->execute(itemUser->getMap());
    }
}

ItemClass::~ItemClass()
{
    while (mEffects.begin() != mEffects.end())
    {
        delete mEffects.begin()->second;
        mEffects.erase(mEffects.begin());
    }

    for (std::vector<AttackInfo *>::iterator it = mAttackInfos.begin(),
         it_end = mAttackInfos.end();
         it != it_end; ++it)
    {
        delete *it;
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

    std::multimap<ItemTriggerType, ItemEffectInfo *>::iterator it, it_end;

    bool ret = false;
    for (it = mEffects.begin(), it_end = mEffects.end(); it != it_end; ++it)
        if (it->first == trigger)
            if (it->second->apply(itemUser))
                ret = true;

    for (it = mDispells.begin(), it_end = mDispells.end(); it != it_end; ++it)
        if (it->first == trigger)
            it->second->dispell(itemUser);

    return ret;
}

void ItemClass::addAttack(AttackInfo *attackInfo,
                             ItemTriggerType applyTrigger,
                             ItemTriggerType dispellTrigger)
{
    mAttackInfos.push_back(attackInfo);
    addEffect(new ItemEffectAttack(attackInfo), applyTrigger, dispellTrigger);
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
