/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#include "game-server/item.hpp"

#include "game-server/being.hpp"

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

bool ItemClass::use(Being *itemUser)
{
    if (mType != ITEM_USABLE) return false;

    mModifiers.applyAttributes(itemUser);
    return true;
}
