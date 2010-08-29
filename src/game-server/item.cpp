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

#include "game-server/item.hpp"

#include "common/configuration.hpp"
#include "game-server/autoattack.hpp"
#include "game-server/attributemanager.hpp"
#include "game-server/being.hpp"
#include "game-server/state.hpp"
#include "scripting/script.hpp"

bool ItemEffectInfo::apply(Being *itemUser)
{
    LOG_WARN("Virtual defintion used in effect application!");
    return false;
}

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
                             mId, mDuration);
}

bool ItemEffectAutoAttack::apply(Being *itemUser)
{
    // TODO - STUB
    return false;
}

void ItemEffectAutoAttack::dispell(Being *itemUser)
{
    // TODO
}

bool ItemEffectScript::apply(Being *itemUser)
{
    // TODO
    return false;
}

void ItemEffectScript::dispell(Being *itemUser)
{
    // TODO
}

bool ItemClass::useTrigger(Being *itemUser, ItemTriggerType trigger)
{
    if (!trigger) return false;
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
    mLifetime = Configuration::getValue("floorItemDecayTime", 0) * 10;
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
