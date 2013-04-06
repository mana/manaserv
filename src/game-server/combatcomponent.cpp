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

#include "combatcomponent.h"

#include "game-server/being.h"
#include "game-server/mapcomposite.h"

#include "utils/logger.h"

CombatComponent::CombatComponent(Being &being):
    mTarget(0),
    mCurrentAttack(0)
{
    being.signal_died.connect(sigc::mem_fun(this,
                               &CombatComponent::diedOrRemoved));
    being.signal_removed.connect(sigc::mem_fun(this,
                                  &CombatComponent::diedOrRemoved));
}

CombatComponent::~CombatComponent()
{
}

void CombatComponent::update(Entity &entity)
{
    // Temporary for as long as Being is not split into Components
    // Prevents to implement all at once
    // TODO: remove this as soon as possible
    Being &being = static_cast<Being&>(entity);


    if (being.getAction() != ATTACK || !mTarget)
            return;

    std::vector<Attack *> attacksReady;
    mAttacks.getUsuableAttacks(&attacksReady);

    if (Attack *triggerableAttack = mAttacks.getTriggerableAttack())
    {
        processAttack(being, *triggerableAttack);
        mAttacks.markAttackAsTriggered();
    }

    // Deal with the ATTACK action.
    if (attacksReady.empty())
        return;

    Attack *highestPriorityAttack = 0;
    // Performs all ready attacks.
    for (std::vector<Attack *>::const_iterator it = attacksReady.begin(),
         it_end = attacksReady.end(); it != it_end; ++it)
    {
        // check if target is in range using the pythagorean theorem
        int distx = being.getPosition().x - mTarget->getPosition().x;
        int disty = being.getPosition().y - mTarget->getPosition().y;
        int distSquare = (distx * distx + disty * disty);
        AttackInfo *info = (*it)->getAttackInfo();
        int maxDist = info->getDamage().range + being.getSize();

        if (distSquare <= maxDist * maxDist &&
                (!highestPriorityAttack ||
                 highestPriorityAttack->getAttackInfo()->getPriority()
                 < info->getPriority()))
        {
            highestPriorityAttack = *it;
        }
    }
    if (highestPriorityAttack)
    {
        mAttacks.startAttack(highestPriorityAttack);
        mCurrentAttack = highestPriorityAttack;
        being.setDestination(being.getPosition());
        // TODO: Turn into direction of enemy
        being.raiseUpdateFlags(UPDATEFLAG_ATTACK);
    }
}

/**
 * Takes a damage structure, computes the real damage based on the
 * stats, deducts the result from the hitpoints and adds the result to
 * the HitsTaken list.
 */
int CombatComponent::damage(Being &target, Being *source, const Damage &damage)
{
    int HPloss = damage.base;
    if (damage.delta)
        HPloss += rand() * (damage.delta + 1) / RAND_MAX;

    // TODO magical attacks and associated elemental modifiers
    switch (damage.type)
    {
        case DAMAGE_PHYSICAL:
            if (!damage.trueStrike &&
                rand()%((int) target.getModifiedAttribute(ATTR_DODGE) + 1) >
                    rand()%(damage.cth + 1))
            {
                HPloss = 0;
                // TODO Process triggers for a dodged physical attack here.
                // If there is an attacker included, also process triggers for the attacker (failed physical strike)
            }
            else
            {
                HPloss = HPloss * (1.0 - (0.0159375f *
                                  target.getModifiedAttribute(ATTR_DEFENSE)) /
                                   (1.0 + 0.017 *
                                 target.getModifiedAttribute(ATTR_DEFENSE))) +
                         (rand()%((HPloss >> 4) + 1));
                // TODO Process triggers for receiving damage here.
                // If there is an attacker included, also process triggers for the attacker (successful physical strike)
            }
            break;
        case DAMAGE_MAGICAL:
#if 0
            target.getModifiedAttribute(BASE_ELEM_BEGIN + damage.element);
#else
            LOG_WARN("Attempt to use magical type damage! This has not been"
                      "implemented yet and should not be used!");
            HPloss = 0;
#endif
            break;
        case DAMAGE_DIRECT:
            break;
        default:
            LOG_WARN("Unknown damage type '" << damage.type << "'!");
            break;
    }

    if (HPloss > 0)
    {
        mHitsTaken.push_back(HPloss);
        const Attribute *HP = target.getAttribute(ATTR_HP);
        LOG_DEBUG("Being " << target.getPublicID() << " suffered " << HPloss
                  << " damage. HP: "
                  << HP->getModifiedAttribute() << "/"
                  << target.getModifiedAttribute(ATTR_MAX_HP));
        target.setAttribute(ATTR_HP, HP->getBase() - HPloss);
        // No HP regen after being hit if this is set.
        // TODO: Reenable this once the attributes are available as a component
        // A bit too fuzzy to implement at the moment
        //mHealthRegenerationTimeout.setSoft(
        //            Configuration::getValue("game_hpRegenBreakAfterHit", 0));
    }
    else
    {
        HPloss = 0;
    }

    signal_damaged.emit(source, damage, HPloss);
    return HPloss;
}

/**
 * Performs an attack
 */
void CombatComponent::processAttack(Being &source, Attack &attack)
{
    performAttack(source, attack.getAttackInfo()->getDamage());
}

/**
 * Adds an attack to the available attacks
 */
void CombatComponent::addAttack(AttackInfo *attackInfo)
{
    mAttacks.add(this, attackInfo);
}

/**
 * Removes an attack from the available attacks
 */
void CombatComponent::removeAttack(AttackInfo *attackInfo)
{
    mAttacks.remove(this, attackInfo);
}

/**
 * Performs an attack.
 */
int CombatComponent::performAttack(Being &source, const Damage &dmg)
{
    // check target legality
    if (!mTarget
            || mTarget == &source
            || mTarget->getAction() == DEAD
            || !mTarget->canFight())
        return -1;

    if (source.getMap()->getPvP() == PVP_NONE
            && mTarget->getType() == OBJECT_CHARACTER
            && source.getType() == OBJECT_CHARACTER)
        return -1;

    return mTarget->getComponent<CombatComponent>()->damage(*mTarget,
                                                              &source, dmg);
}
