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

#include "game-server/monstercombatcomponent.h"

#include "game-server/monster.h"
#include "scripting/scriptmanager.h"

MonsterCombatComponent::MonsterCombatComponent(Entity &monster,
                                               MonsterClass *specy):
    CombatComponent(monster),
    mDamageMutation(0)
{
    for (auto &attack : specy->getAttackInfos())
    {
        addAttack(attack);
    }
}

/**
 * Performs an attack
 */
void MonsterCombatComponent::processAttack(Entity *source, Attack &attack)
{
    if (!mTarget)
    {
        source->getComponent<BeingComponent>()->setAction(*source, STAND);
        return;
    }

    Damage dmg = attack.getAttackInfo()->getDamage();
    dmg.skill   = 0;
    dmg.base    *= mDamageMutation;
    dmg.delta   *= mDamageMutation;

    int hit = performAttack(*mTarget, attack.getAttackInfo()->getDamage());

    const Script::Ref &scriptCallback =
            attack.getAttackInfo()->getScriptCallback();

    if (scriptCallback.isValid() && hit > -1)
    {
        Script *script = ScriptManager::currentState();
        script->prepare(scriptCallback);
        script->push(source);
        script->push(mTarget);
        script->push(hit);
        script->execute(source->getMap());
    }
}

/**
 * Calls the damage function in Being and updates the aggro list
 */
int MonsterCombatComponent::damage(Entity &target,
                                   Entity *source,
                                   const Damage &damage)
{
    Damage newDamage = damage;
    MonsterClass *specy = target.getComponent<MonsterComponent>()->getSpecy();
    float factor = specy->getVulnerability(newDamage.element);
    newDamage.base = newDamage.base * factor;
    newDamage.delta = newDamage.delta * factor;
    int hpLoss = CombatComponent::damage(target, source, newDamage);


    if (specy->getDamageCallback().isValid())
    {
        Script *script = ScriptManager::currentState();
        script->prepare(specy->getDamageCallback());
        script->push(&target);
        script->push(source);
        script->push(hpLoss);
        // TODO: add exact damage parameters as well
        script->execute(target.getMap());
    }
    return hpLoss;
}
