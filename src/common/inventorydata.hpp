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

#ifndef COMMON_INVENTORYDATA_HPP
#define COMMON_INVENTORYDATA_HPP

#include <vector>

/**
 * Numbers of inventory slots
 */

enum
{
    EQUIPMENT_SLOTS = 11,
    INVENTORY_SLOTS = 50
};

/**
 * Structure storing an item in the inventory.
 * When the itemId is zero, this item represents "amount" consecutive empty slots.
 */

struct InventoryItem
{
    unsigned short itemId;
    unsigned char amount;
};

/**
 * Structure storing the equipment and inventory of a Player.
 */
struct Possessions
{
    std::vector< InventoryItem > inventory;
    int money;
    unsigned short equipment[EQUIPMENT_SLOTS];
    Possessions(): money(0)
    { for (int i = 0; i < EQUIPMENT_SLOTS; ++i) equipment[i] = 0; }
};

#endif
