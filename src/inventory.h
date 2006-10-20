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
 *  $Id: items.h 2636 2006-09-02 12:03:22Z gmelquio $
 */

#ifndef INVENTORY_H
#define INVENTORY_H

#include "itemmanager.h"

// items in inventory :
const unsigned char MAX_ITEMS_IN_INVENTORY = 50, // Max 254.
// Equipment rules:
// 1 Brest equipment
// 1 arms equipment
// 1 head equipment
// 1 legs equipment
// 1 feet equipment
// 2 rings
// 1 necklace
// Fight:
// 2 one-handed weapons
// or 1 two-handed weapon
// or 1 one-handed weapon + 1 shield.
// = 10 total slots for equipment.
                    TOTAL_EQUIPMENT_SLOTS = 10,
                    INVENTORY_FULL = 255;

/**
 * Stored Item only contains id reference to items
 * in the order not to carry every item info for each carried items
 * in the inventory.
 * Also contains amount.
 */
struct StoredItem
{
    unsigned int itemId;
    unsigned char amount;
};

/**
 * Equipped items that keeps which kind of item is in equipment.
 */
struct EquippedItem
{
    unsigned int itemId;
    short itemType;
};

/**
 * Class used to store minimal info on player's inventories
 * to keep it fast.
 * See Item and ItemManager to get more info on an item.
 */
class Inventory
{
    public:
        /**
         * ctor. Add slot spaces to MAX_ITEMS_IN_INVENTORY.
         */
        Inventory();

        /**
         * Convenience function to get slot from ItemId.
         * If more than one occurence is found, the first is given.
         */
        unsigned char
        getSlotFromId(const unsigned int itemId);

        /**
         * Return StoredItem
         */
        StoredItem
        getStoredItemAt(unsigned char slot) const { return itemList.at(slot); };

        /**
         * Return Item reference from ItemReference
         */
        //ItemPtr getItem(unsigned short index) const
        //{ return itemReference.getItem(itemList.at(index).itemId); };

        /**
         * Search in inventory only if an item is present.
         * Don't tell if an item is equipped.
         */
        bool
        hasItem(unsigned int itemId);

        /**
         * Tells an item's amount
         */
        unsigned short
        getItemAmount(unsigned char slot) const { return itemList.at(slot).amount; };

        /**
         * Return Item reference Id
         */
        unsigned int
        getItemId(unsigned char slot) const { return itemList.at(slot).itemId; };

        /**
         * add an item with amount 
         * (don't create it if amount was 0)
         * @return short value: Indicates the number of items added.
         */
        short
        addItem(unsigned int itemId, unsigned char amount = 1);

        /**
         * Remove an item searched by ItemId.
         * Delete if amount = 0.
         * @return short value: Indicates the number of items removed.
         * This function removes the given amount using every slots
         * if necessary.
         */
        short
        removeItem(unsigned int itemId, unsigned char amount = 0);

        /**
         * Remove an item searched by slot index.
         * Delete if amount = 0.
         * @return short value: Indicates the number of items removed.
         * Removes only in the given slot.
         */
        short
        removeItem(unsigned char slot, unsigned char amount = 0);

        /**
         * Equip an item searched by its id.
         */
        bool
        equipItem(unsigned int itemId);

        /**
         * Unequip an item searched by its id.
         */
        bool
        unequipItem(unsigned int itemId);

        /**
         * Equip an item searched by its slot index.
         */
        bool
        equipItem(unsigned char slot);

        /**
         * Unequip an item searched by its slot index.
         */
        bool
        unequipItem(unsigned char slot);

        /**
         * The function called to use an item applying
         * only the modifiers (for simple items...)
         */
        bool
        use(unsigned char slot, BeingPtr itemUser);

        /**
         * The function called to use an item applying
         * only the modifiers (for simple items...)
         */
        bool
        use(unsigned int itemId, BeingPtr itemUser);

    private:

        /**
         * Give the first free slot number in itemList.
         */
        unsigned char getInventoryFreeSlot();

        // Stored items in inventory and equipment
        std::vector<StoredItem> itemList; /**< Items in inventory */
        std::vector<EquippedItem> equippedItemList; /**< Equipped Items */
        /**
         * Used to know which type of arrow is used with a bow,
         * for instance
         */
        StoredItem equippedProjectiles;
};

/**
 * Type definition for a smart pointer to Being.
 */
typedef utils::CountedPtr<Inventory> InventoryPtr;

#endif
