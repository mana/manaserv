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

#ifndef INVENTORY_H
#define INVENTORY_H

#include "game-server/being.hpp"

enum
{
// items in inventory :
    MAX_ITEMS_IN_INVENTORY = 50, // Max 252.
// Equipment rules:
// 1 Brest equipment
    EQUIP_BREST_SLOT = 0,
// 1 arms equipment
    EQUIP_ARMS_SLOT = 1,
// 1 head equipment
    EQUIP_HEAD_SLOT = 2,
// 1 legs equipment
    EQUIP_LEGS_SLOT = 3,
// 1 feet equipment
    EQUIP_FEET_SLOT = 4,
// 2 rings
    EQUIP_RING1_SLOT = 5,
    EQUIP_RING2_SLOT = 6,
// 1 necklace
    EQUIP_NECKLACE_SLOT = 7,
// Fight:
// 2 one-handed weapons
    EQUIP_FIGHT1_SLOT = 8,
    EQUIP_FIGHT2_SLOT = 9,
// or 1 two-handed weapon
// or 1 one-handed weapon + 1 shield.
//  Projectiles
    EQUIP_PROJECTILES_SLOT = 10,
// = 10 total slots for equipment.
    TOTAL_EQUIPMENT_SLOTS = 11,
// Error codes
    NOT_EQUIPPABLE = 253,
    NO_ITEM_TO_EQUIP = 254,
    INVENTORY_FULL = 255
};

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
         * Convenience function to get slot from ItemId.
         * If more than one occurence is found, the first is given.
         */
        unsigned char
        getSlotFromId(unsigned int itemId);

        /**
         * Return StoredItem
         */
        StoredItem
        getStoredItemAt(unsigned char slot) const { return itemList[slot]; };

        /**
         * Search in inventory and equipment if an item is present.
         */
        bool
        hasItem(unsigned int itemId,
                bool searchInInventory = true,
                bool searchInEquipment = true);

        /**
         * Tells an item's amount
         */
        unsigned short
        getItemAmount(unsigned char slot) const { return itemList[slot].amount; };

        /**
         * Return Item reference Id
         */
        unsigned int
        getItemId(unsigned char slot) const { return itemList[slot].itemId; };

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
         * Can equip more than one item at a time.
         * @return unsigned char value: Returns the slot if successful
         * or the error code if not.
         */
        unsigned char
        equipItem(unsigned int itemId);

        /**
         * Unequip an item searched by its id.
         * Can unequip more than one item at a time.
         */
        bool
        unequipItem(unsigned int itemId);

        /**
         * Equip an item searched by its slot index.
         */
        bool
        equipItem(unsigned char inventorySlot, unsigned char equipmentSlot);

        /**
         * Unequip an equipped item searched by its slot index.
         */
        bool
        unequipItem(unsigned char inventorySlot, unsigned char equipmentSlot);

        /**
         * The function called to use an item applying
         * only the modifiers
         */
        bool
        use(unsigned char slot, BeingPtr itemUser);

        /**
         * The function called to use an item applying
         * only the modifiers
         */
        bool
        use(unsigned int itemId, BeingPtr itemUser);

    private:

        /**
         * Give the first free slot number in itemList.
         */
        unsigned char getInventoryFreeSlot();

        /**
         * Quick equip an equipment with a given equipSlot,
         * an itemId and an itemType.
         * @return the equipment slot if successful,
         * the error code, if not.
         */
        unsigned char equipItem_(unsigned int itemId,
                                 unsigned int itemType,
                                 unsigned char equipmentSlot);

        /**
         * Quick unequip an equipment with a given equipSlot,
         * and an itemId.
         * @return the Equipment slot if successful,
         * the error code, if not.
         */
        unsigned char unequipItem_(unsigned int itemId,
                                   unsigned char equipmentSlot);


        // Stored items in inventory and equipment
        std::vector<StoredItem> itemList; /**< Items in inventory */
        std::vector<EquippedItem> equippedItemList; /**< Equipped Items */
        /**
         * Used to know which type of arrow is used with a bow,
         * for instance
         */
        StoredItem equippedProjectiles;
};

#endif
