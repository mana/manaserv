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
 * TODO: Make this configurable and sent to the client.
 */
#define INVENTORY_SLOTS 50

/**
 * Structure storing an item in the inventory.
 */
struct InventoryItem
{
    InventoryItem():
        itemId(0), amount(0)
    {}

    unsigned int itemId;
    unsigned int amount;
};

struct EquipmentItem
{
    EquipmentItem():
        itemId(0), itemInstance(0)
    {}

    EquipmentItem(unsigned int itemId, unsigned int itemInstance)
    {
        this->itemId = itemId;
        this->itemInstance = itemInstance;
    }

    // The item id taken from the item db.
    unsigned int itemId;
    // A unique instance number used to separate items when equipping the same
    // item id multiple times on possible multiple slots.
    unsigned int itemInstance;
};

// inventory slot id -> { item }
typedef std::map< unsigned int, InventoryItem > InventoryData;

// equip slot id -> { item id, item instance }
// Equipment taking up multiple equip slot ids will be referenced multiple times
typedef std::multimap< unsigned int, EquipmentItem > EquipData;

/**
 * Structure storing the equipment and inventory of a Player.
 */
struct Possessions
{
    friend class Inventory;
public:
    const EquipData &getEquipment() const
    { return equipSlots; }

    const InventoryData &getInventory() const
    { return inventory; }

    /**
     * Should be done only at character serialization and storage load time.
     */
    void setEquipment(EquipData &equipData)
    { equipSlots.swap(equipData); }
    void setInventory(InventoryData &inventoryData)
    { inventory.swap(inventoryData); }

private:
    InventoryData inventory;
    EquipData equipSlots;
};

#endif
