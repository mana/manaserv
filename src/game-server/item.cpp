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

#include "game-server/being.hpp"
#include "scripting/script.hpp"

ItemType itemTypeFromString (const std::string &name)
{
    static std::map<const std::string, ItemType> table;

    if (table.empty())
    {
        table["generic"]        = ITEM_UNUSABLE;
        table["usable"]         = ITEM_USABLE;
        table["equip-1hand"]    = ITEM_EQUIPMENT_ONE_HAND_WEAPON;
        table["equip-2hand"]    = ITEM_EQUIPMENT_TWO_HANDS_WEAPON;
        table["equip-torso"]    = ITEM_EQUIPMENT_TORSO;
        table["equip-arms"]     = ITEM_EQUIPMENT_ARMS;
        table["equip-head"]     = ITEM_EQUIPMENT_HEAD;
        table["equip-legs"]     = ITEM_EQUIPMENT_LEGS;
        table["equip-shield"]   = ITEM_EQUIPMENT_SHIELD;
        table["equip-ring"]     = ITEM_EQUIPMENT_RING;
        table["equip-necklace"] = ITEM_EQUIPMENT_NECKLACE;
        table["equip-feet"]     = ITEM_EQUIPMENT_FEET;
        table["equip-ammo"]     = ITEM_EQUIPMENT_AMMO;
        table["hairsprite"]     = ITEM_HAIRSPRITE;
        table["racesprite"]     = ITEM_RACESPRITE;
    }

    std::map<const std::string, ItemType>::iterator val = table.find(name);

    return val == table.end() ? ITEM_UNKNOWN : (*val).second;
}

int ItemModifiers::getValue(int type) const
{
    for (std::vector< ItemModifier >::const_iterator i = mModifiers.begin(),
         i_end = mModifiers.end(); i != i_end; ++i)
    {
        if (i->type == type) return i->value;
    }
    return 0;
}

int ItemModifiers::getAttributeValue(int attr) const
{
    return getValue(MOD_ATTRIBUTE + attr);
}

void ItemModifiers::setValue(int type, int value)
{
    if (value)
    {
        ItemModifier m;
        m.type = type;
        m.value = value;
        mModifiers.push_back(m);
    }
}

void ItemModifiers::setAttributeValue(int attr, int value)
{
    setValue(MOD_ATTRIBUTE + attr, value);
}

void ItemModifiers::applyAttributes(Being *b) const
{
    /* Note: if someone puts a "lifetime" property on an equipment, strange
       behavior will occur, as its effect will be canceled twice. While this
       could be desirable for some "cursed" items, it is probably an error
       that should be detected somewhere else. */
    int lifetime = getValue(MOD_LIFETIME);
    for (std::vector< ItemModifier >::const_iterator i = mModifiers.begin(),
         i_end = mModifiers.end(); i != i_end; ++i)
    {
        if (i->type < MOD_ATTRIBUTE) continue;
        b->applyModifier(i->type - MOD_ATTRIBUTE, i->value, lifetime);
    }
}

void ItemModifiers::cancelAttributes(Being *b) const
{
    for (std::vector< ItemModifier >::const_iterator i = mModifiers.begin(),
         i_end = mModifiers.end(); i != i_end; ++i)
    {
        if (i->type < MOD_ATTRIBUTE) continue;
        b->applyModifier(i->type - MOD_ATTRIBUTE, -i->value);
    }
}

ItemClass::~ItemClass()
{
    if (mScript) delete mScript;
}

bool ItemClass::use(Being *itemUser)
{
    if (mType != ITEM_USABLE) return false;
    if (mScript)
    {
       mScript->setMap(itemUser->getMap());
       mScript->prepare("use");
       mScript->push(itemUser);
       mScript->push(mDatabaseID);  // ID of the item
       mScript->execute();
    }
    mModifiers.applyAttributes(itemUser);
    return true;
}
