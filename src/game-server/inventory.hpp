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
    int itemId;
    unsigned char amount;
};

/**
 * Equipped items that keeps which kind of item is in equipment.
 */
struct EquippedItem
{
    int itemId;
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
        int getSlotFromId(int itemId);

        /**
         * Returns item.
         */
        StoredItem const &getStoredItemAt(int slot) const
        { return itemList[slot]; };

        /**
         * Looks in inventory and equipment whether an item is present or not.
         */
        bool hasItem(int itemId,
                     bool searchInInventory = true,
                     bool searchInEquipment = true);

        /**
         * Tells an item's amount
         */
        int getItemAmount(int slot) const
        { return itemList[slot].amount; };

        /**
         * Returns item reference ID.
         */
        int getItemId(int slot) const
        { return itemList[slot].itemId; };

        /**
         * Adds a given amount of items.
         * @return Number of items really added.
         */
        int insertItem(int itemId, int amount = 1);

        /**
         * Removes an item given by ID.
         * @return Number of items really removed.
         */
        int removeItemById(int itemId, int amount);

        /**
         * Removes an item given by slot.
         * @return Number of items really removed.
         */
        int removeItemBySlot(int slot, int amount);

        /**
         * Equip an item searched by its id.
         * Can equip more than one item at a time.
         * @return unsigned char value: Returns the slot if successful
         * or the error code if not.
         */
        int equipItem(int itemId);

        /**
         * Unequip an item searched by its id.
         * Can unequip more than one item at a time.
         */
        bool unequipItem(int itemId);

        /**
         * Equips an item searched by its slot index.
         */
        bool equipItem(int inventorySlot, int equipmentSlot);

        /**
         * Unequips an equipped item searched by its slot index.
         */
        bool unequipItem(int inventorySlot, int equipmentSlot);

    private:

        /**
         * Gives the first free slot number in itemList.
         */
        int getInventoryFreeSlot();

        /**
         * Quick equip an equipment with a given equipSlot,
         * an itemId and an itemType.
         * @return the equipment slot if successful,
         * the error code, if not.
         */
        int equipItem_(int itemId,
                       int itemType,
                       int equipmentSlot);

        /**
         * Quick unequip an equipment with a given equipSlot,
         * and an itemId.
         * @return the Equipment slot if successful,
         * the error code, if not.
         */
        int unequipItem_(int itemId,
                         int equipmentSlot);


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
