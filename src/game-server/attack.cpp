/*
 *  The Mana Server
 *  Copyright (C) 2010  The Mana Development Team
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

#include "attack.h"

#include <cassert>

#include "common/defines.h"

#include "game-server/character.h"
#include "game-server/skillmanager.h"

AttackInfo *AttackInfo::readAttackNode(xmlNodePtr node)
{
    std::string skill = XML::getProperty(node, "skill", std::string());

    unsigned skillId;
    if (utils::isNumeric(skill))
        skillId = utils::stringToInt(skill);
    else
        skillId = skillManager->getId(skill);

    if (!skill.empty() && !skillManager->exists(skillId))
    {
        LOG_WARN("Error parsing Attack node: Invalid skill " << skill
                 << " taking default skill");
        skillId = skillManager->getDefaultSkillId();
    }

    unsigned id = XML::getProperty(node, "id", 0);
    unsigned priority = XML::getProperty(node, "priority", 0);
    unsigned warmupTime = XML::getProperty(node, "warmuptime", 0);
    unsigned cooldownTime = XML::getProperty(node, "cooldowntime", 0);
    unsigned reuseTime = XML::getProperty(node, "reusetime", 0);
    unsigned short baseDamange = XML::getProperty(node, "basedamage", 0);
    unsigned short deltaDamage = XML::getProperty(node, "deltadamage", 0);
    unsigned short chanceToHit = XML::getProperty(node, "chancetohit", 0);
    unsigned short range = XML::getProperty(node, "range", 0);
    Element element = elementFromString(
            XML::getProperty(node, "element", "neutral"));
    DamageType type = damageTypeFromString(
            XML::getProperty(node, "type", "other"));

    Damage dmg;
    dmg.id = id;
    dmg.skill = skillId;
    dmg.base = baseDamange;
    dmg.delta = deltaDamage;
    dmg.cth = chanceToHit;
    dmg.range = range;
    dmg.element = element;
    dmg.type = type;
    AttackInfo *attack = new AttackInfo(priority, dmg, warmupTime, cooldownTime,
                                        reuseTime);
    return attack;
}

void Attacks::add(CombatComponent *combatComponent, AttackInfo *attackInfo)
{
    mAttacks.push_back(Attack(attackInfo));
    attack_added.emit(combatComponent, *mAttacks.rbegin());
}

void Attacks::remove(CombatComponent *combatComponent, AttackInfo *attackInfo)
{
    for (std::vector<Attack>::iterator it = mAttacks.begin(),
         it_end = mAttacks.end(); it != it_end; ++it)
    {
        if ((*it).getAttackInfo() == attackInfo)
        {
            if (mCurrentAttack && mCurrentAttack->getAttackInfo() == attackInfo)
                mCurrentAttack = 0;
            attack_removed.emit(combatComponent, *it);
            mAttacks.erase(it);
            return;
        }
    }
}

void Attacks::markAttackAsTriggered()
{
    mCurrentAttack->markAsTriggered();
    mCurrentAttack = 0;
}

Attack *Attacks::getTriggerableAttack()
{
    if (!mCurrentAttack)
        return 0;

    int cooldownTime = mCurrentAttack->getAttackInfo()->getCooldownTime();
    if (mAttackTimer.remaining() <= cooldownTime)
    {
        return mCurrentAttack;
    }

    return 0;
}

void Attacks::startAttack(Attack *attack)
{
    mCurrentAttack = attack;
    mAttackTimer.set(attack->getAttackInfo()->getWarmupTime() +
                     attack->getAttackInfo()->getCooldownTime());
}

void Attacks::getUsuableAttacks(std::vector<Attack *> *ret)
{
    assert(ret != 0);

    // we have a current Attack
    if ((!mAttackTimer.expired() && mCurrentAttack))
        return;
    for (std::vector<Attack>::iterator it = mAttacks.begin();
         it != mAttacks.end(); ++it)
    {
        Attack &attack = *it;

        if (attack.isUsuable())
        {
            ret->push_back(&attack);
        }
    }
}
