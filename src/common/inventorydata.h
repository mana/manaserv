/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
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

#ifndef COMMON_INVENTORYDATA_H
#define COMMON_INVENTORYDATA_H

#include <vector>
#include <map>

/**
 * Numbers of inventory slots
 */

enum
{
    INVENTORY_SLOTS = 50
};

/**
 * Structure storing an item in the inventory.
 * When the itemId is zero, this item represents "amount" consecutive empty slots.
 */

struct InventoryItem
{
    unsigned int itemId;
    unsigned int amount;
};
// slot id -> { item }
typedef std::map< unsigned short, InventoryItem > InventoryData;
// equip slot type -> { slot ids }
// Equipment taking up multiple slots will be referenced multiple times
typedef std::multimap< unsigned int, unsigned int > EquipData;

/**
 * Structure storing the equipment and inventory of a Player.
 */
struct Possessions
{
    InventoryData inventory;
    EquipData equipSlots;
};

#endif
