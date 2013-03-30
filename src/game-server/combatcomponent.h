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

#ifndef COMBATCOMPONENT_H
#define COMBATCOMPONENT_H

#include "component.h"

#include <vector>

#include <sigc++/trackable.h>

#include "game-server/attack.h"

class Actor;
class Being;

/**
 * Type definition for a list of hits
 */
typedef std::vector<unsigned> Hits;

class CombatComponent: public Component, public sigc::trackable
{
public:
    static const ComponentType type = CT_Fighting;

    CombatComponent(Being &being);
    virtual ~CombatComponent();

    void update(Entity &entity);

    void addAttack(AttackInfo *attack);
    void removeAttack(AttackInfo *attackInfo);
    Attacks &getAttacks();

    const Hits &getHitsTaken() const;
    void clearHitsTaken();

    int performAttack(Being &source, const Damage &dmg);
    virtual int damage(Being &target, Being *source, const Damage &damage);

    int getAttackId() const;

    Being *getTarget() const;
    void setTarget(Being *target);
    void clearTarget();

    void diedOrRemoved(Entity *entity);

    sigc::signal<void, Being *, const Damage &, int> signal_damaged;

protected:
    virtual void processAttack(Being &source, Attack &attack);

    Being *mTarget;
    Attacks mAttacks;
    Attack *mCurrentAttack;     // Last used attack
    Hits mHitsTaken;            //List of punches taken since last update.

};

inline Attacks &CombatComponent::getAttacks()
{
    return mAttacks;
}

/**
 * Gets the damage list.
 */
inline const Hits &CombatComponent::getHitsTaken() const
{
    return mHitsTaken;
}

/**
 * Clears the damage list.
 */
inline void CombatComponent::clearHitsTaken()
{
    mHitsTaken.clear();
}

/**
 * Gets the attack id the being is currently performing.
 * For being, this is defaulted to the first one (1).
 */
inline int CombatComponent::getAttackId() const
{
    return mCurrentAttack ?
            mCurrentAttack->getAttackInfo()->getDamage().id : 0;
}

/**
 * Get Target
 */
inline Being *CombatComponent::getTarget() const
{
    return mTarget;
}

/**
 * Set Target
 */
inline void CombatComponent::setTarget(Being *target)
{
    mTarget = target;
}

/**
 * Clears the target
 */
inline void CombatComponent::clearTarget()
{
    mTarget = 0;
}

/**
 * Handler for the died and removed event of the targeting being
 * @param entity The removed/died being (not used here)
 */
inline void CombatComponent::diedOrRemoved(Entity *entity)
{
    clearTarget();
}

#endif // COMBATCOMPONENT_H
