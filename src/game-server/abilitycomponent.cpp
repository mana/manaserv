/*
 *  The Mana Server
 *  Copyright (C) 2013 The Mana Developers
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

#include "abilitycomponent.h"

#include "game-server/being.h"
#include "game-server/entity.h"

#include "scripting/scriptmanager.h"

#include "utils/logger.h"

AbilityComponent::AbilityComponent(Entity &entity):
    mLastUsedAbilityId(0),
    mLastTargetBeingId(0)
{
    entity.getComponent<BeingComponent>()->signal_attribute_changed.connect(
            sigc::mem_fun(this, &AbilityComponent::attributeChanged));
}

void AbilityComponent::update(Entity &entity)
{
    // Update ability recharge
    for (auto &it : mAbilities)
    {
        AbilityValue &s = it.second;
        if (s.abilityInfo->rechargeable &&
            s.currentPoints < s.abilityInfo->neededPoints)
        {
            auto *beingComponent = entity.getComponent<BeingComponent>();
            const double rechargeSpeed = beingComponent->getModifiedAttribute(
                    s.abilityInfo->rechargeAttribute);
            s.currentPoints += (int)rechargeSpeed;
            if (s.currentPoints >= s.abilityInfo->neededPoints &&
                    s.abilityInfo->rechargedCallback.isValid())
            {
                Script *script = ScriptManager::currentState();
                script->prepare(s.abilityInfo->rechargedCallback);
                script->push(&entity);
                script->push(s.abilityInfo->id);
                script->execute(entity.getMap());
            }
        }
    }

}

/**
 * Removes an available ability action
 */
bool AbilityComponent::takeAbility(int id)
{
    AbilityMap::iterator i = mAbilities.find(id);
    if (i != mAbilities.end())
    {
        mAbilities.erase(i);
        signal_ability_took.emit(id);
        return true;
    }
    return false;
}

bool AbilityComponent::abilityUseCheck(AbilityMap::iterator it)
{
    if (!mCooldown.expired())
        return false;

    if (it == mAbilities.end())
    {
        LOG_INFO("Entity uses ability " << it->first
                 << " without authorization.");
        return false;
    }

    //check if the ability is currently recharged
    AbilityValue &ability = it->second;
    if (ability.abilityInfo->rechargeable &&
            ability.currentPoints < ability.abilityInfo->neededPoints)
    {
        LOG_INFO("Character uses ability " << it->first
                 << " which is not recharged. ("
                 << ability.currentPoints << "/"
                 << ability.abilityInfo->neededPoints << ")");
        return false;
    }

    if (!ability.abilityInfo->useCallback.isValid())
    {
        LOG_WARN("No callback for use of ability "
                 << ability.abilityInfo->categoryName << "/"
                 << ability.abilityInfo->name << ". Ignoring ability.");
        return false;
    }
    return true;
}

/**
 * makes the character perform a ability on a being
 * when it is allowed to do so
 */
bool AbilityComponent::useAbilityOnBeing(Entity &user, int id, Entity *b)
{
    AbilityMap::iterator it = mAbilities.find(id);
    if (!abilityUseCheck(it))
            return false;
    AbilityValue &ability = it->second;

    if (ability.abilityInfo->target != AbilityManager::TARGET_BEING)
        return false;

    if (ability.abilityInfo->autoconsume) {
        ability.currentPoints = 0;
        signal_ability_changed.emit(id);
        startCooldown(user, ability.abilityInfo);
    }

    //tell script engine to cast the spell
    Script *script = ScriptManager::currentState();
    script->prepare(ability.abilityInfo->useCallback);
    script->push(&user);
    script->push(b);
    script->push(ability.abilityInfo->id);
    script->execute(user.getMap());

    mLastUsedAbilityId = id;
    if (b)
        mLastTargetBeingId = b->getComponent<ActorComponent>()->getPublicID();
    else
        mLastTargetBeingId = 0;
    user.getComponent<ActorComponent>()->raiseUpdateFlags(
            UPDATEFLAG_ABILITY_ON_BEING);
    return true;
}

/**
 * makes the character perform a ability on a map point
 * when it is allowed to do so
 */
bool AbilityComponent::useAbilityOnPoint(Entity &user, int id, int x, int y)
{
    AbilityMap::iterator it = mAbilities.find(id);
    if (!abilityUseCheck(it))
            return false;
    AbilityValue &ability = it->second;

    if (ability.abilityInfo->target != AbilityManager::TARGET_POINT)
        return false;

    if (ability.abilityInfo->autoconsume) {
        ability.currentPoints = 0;
        signal_ability_changed.emit(id);
        startCooldown(user, ability.abilityInfo);
    }

    //tell script engine to cast the spell
    Script *script = ScriptManager::currentState();
    script->prepare(ability.abilityInfo->useCallback);
    script->push(&user);
    script->push(x);
    script->push(y);
    script->push(ability.abilityInfo->id);
    script->execute(user.getMap());

    mLastUsedAbilityId = id;
    mLastTargetPoint = Point(x, y);
    user.getComponent<ActorComponent>()->raiseUpdateFlags(
            UPDATEFLAG_ABILITY_ON_POINT);
    return true;
}

/**
 * Allows a character to perform a ability
 */
bool AbilityComponent::giveAbility(int id, int currentPoints)
{
    if (mAbilities.find(id) == mAbilities.end())
    {
        auto *abilityInfo = abilityManager->getAbilityInfo(id);
        if (!abilityInfo)
        {
            LOG_ERROR("Tried to give not existing ability id " << id << ".");
            return false;
        }
        return giveAbility(abilityInfo, currentPoints);
    }
    return false;
}

bool AbilityComponent::giveAbility(const AbilityManager::AbilityInfo *info,
                                   int currentPoints)
{
    bool added = mAbilities.insert(std::pair<int, AbilityValue>(info->id,
                                   AbilityValue(currentPoints, info))).second;

    signal_ability_changed.emit(info->id);
    return added;
}

/**
 * Sets new current mana + makes sure that the client will get informed.
 */
bool AbilityComponent::setAbilityMana(int id, int mana)
{
    AbilityMap::iterator it = mAbilities.find(id);
    if (it != mAbilities.end())
    {
        it->second.currentPoints = mana;
        signal_ability_changed.emit(id);
        return true;
    }
    return false;
}

void AbilityComponent::startCooldown(
        Entity &entity, const AbilityManager::AbilityInfo *abilityInfo)
{
    unsigned cooldownAttribute = abilityInfo->cooldownAttribute;
    auto *bc = entity.getComponent<BeingComponent>();
    int cooldown = (int)bc->getModifiedAttribute(cooldownAttribute);
    // Enforce a minimum cooldown of 1 tick to prevent syncing issues
    cooldown = std::max(cooldown, 1);
    mCooldown.set(cooldown);
    signal_cooldown_activated.emit();
}

void AbilityComponent::attributeChanged(Entity *entity, unsigned attr)
{
    for (auto &abilityIt : mAbilities)
    {
        // Inform the client about rechargespeed changes
        if (abilityIt.second.abilityInfo->rechargeAttribute == attr)
            signal_ability_changed.emit(abilityIt.first);
    }
}

