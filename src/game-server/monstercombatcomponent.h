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

#ifndef MONSTERCOMBATCOMPONENT_H
#define MONSTERCOMBATCOMPONENT_H

#include "game-server/combatcomponent.h"

#include "game-server/attack.h"
#include "game-server/being.h"

class MonsterClass;

class MonsterCombatComponent: public CombatComponent
{
public:
    MonsterCombatComponent(Being &monster, MonsterClass *specy);

    void processAttack(Being *source, Attack &attack);
    int damage(Being &target, Being *source, const Damage &damage);

    void setDamageMutation(double mutation);

private:
    double mDamageMutation;
};

/**
 * Sets the mutation of the damage compared to the default damage of the specy
 * @param mutation
 */
inline void MonsterCombatComponent::setDamageMutation(double mutation)
{
    mDamageMutation = mutation;
}

#endif /* MONSTERCOMBATCOMPONENT_H */
