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

#ifndef ABILITYCOMPONENT_H_
#define ABILITYCOMPONENT_H_

#include "game-server/abilitymanager.h"
#include "game-server/component.h"
#include "game-server/timeout.h"

#include <sigc++/signal.h>

struct AbilityValue
{
    AbilityValue(unsigned currentMana,
                 const AbilityManager::AbilityInfo *abilityInfo)
        : currentPoints(currentMana)
        , abilityInfo(abilityInfo)
    {}

    unsigned currentPoints;
    const AbilityManager::AbilityInfo *abilityInfo;
};

/**
 * Stores abilities by their id.
 */
typedef std::map<unsigned, AbilityValue> AbilityMap;


class AbilityComponent: public Component
{
public:
    static const ComponentType type = CT_Ability;

    AbilityComponent(Entity &entity);

    void update(Entity &entity);

    void useAbilityOnBeing(Entity &user, int id, Entity *b);
    void useAbilityOnPoint(Entity &user, int id, int x, int y);

    bool giveAbility(int id, int currentMana = 0);
    bool hasAbility(int id) const;
    bool takeAbility(int id);
    AbilityMap::iterator findAbility(int id);
    const AbilityMap &getAbilities() const;
    void clearAbilities();

    bool setAbilityMana(int id, int mana);

    void startCooldown(Entity &entity,
                       const AbilityManager::AbilityInfo *abilityInfo);
    int remainingCooldown() const;

    sigc::signal<void, int> signal_ability_changed;
    sigc::signal<void, int> signal_ability_took;
    sigc::signal<void> signal_cooldown_activated;
private:
    bool abilityUseCheck(AbilityMap::iterator it);
    void attributeChanged(Entity *entity, unsigned attr);

    Timeout mCooldown;

    AbilityMap mAbilities;
};


/**
 * Gets the ability value by id
 */
inline AbilityMap::iterator AbilityComponent::findAbility(int id)
{
    return mAbilities.find(id);
}

/**
 * Removes all abilities from character
 */
inline void AbilityComponent::clearAbilities()
{
    mAbilities.clear();
}

/**
 * Checks if a character knows a ability action
 */
inline bool AbilityComponent::hasAbility(int id) const
{
    return mAbilities.find(id) != mAbilities.end();
}

inline const AbilityMap &AbilityComponent::getAbilities() const
{
    return mAbilities;
}

inline int AbilityComponent::remainingCooldown() const
{
    return mCooldown.remaining();
}

#endif /* ABILITYCOMPONENT_H_ */
