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

#include "item.h"

/**
 * Stored Item only contains id reference to items
 * in the order not to carry every item info for each carried items
 * in the inventory.
 * Also contains amount, and if equipped.
 */
struct StoredItem
{
    unsigned int itemId;
    unsigned short amount;
    bool equipped;
};

/**
 * Class used to store minimal info on player's inventories
 * to keep it fast.
 * See Item and ItemReference to get more info on an item.
 */
class Inventory
{
    public:
        /**
         * ctor.
         */
        Inventory() {}

        /**
         * Convenience function to get index from ItemId.
         * If more than one occurence is found, the first is given.
         */
        unsigned short
        getSlotIndex(const unsigned int itemId);

        /**
         * Return StoredItem
         */
        StoredItem
        getStoredItemAt(unsigned short index) const { return itemList.at(index); };

        /**
         * Return Item reference from ItemReference
         */
        //ItemPtr getItem(unsigned short index) const
        //{ return itemReferencegetItem(itemList.at(index).itemId); };

        /**
         * Tells if an item is equipped
         */
        bool
        isEquipped(unsigned short index) const { return itemList.at(index).equipped; };

        /**
         * Tells an item's amount
         */
        unsigned short
        getItemAmount(unsigned short index) const { return itemList.at(index).amount; };

        /**
         * Return Item reference Id
         */
        unsigned int
        getItemId(unsigned short index) const { return itemList.at(index).itemId; };

        /**
         * add an item with amount 
         * (creates it non-equipped if amount was 0)
         */
        bool
        addItem(unsigned int itemId, unsigned short amount);

        /**
         * Remove an item searched by ItemId.
         * Delete if amount = 0.
         */
        bool
        removeItem(unsigned int itemId, unsigned short amount = 0);

        /**
         * Remove an item searched by slot index.
         * Delete if amount = 0.
         */
        bool
        removeItem(unsigned short index, unsigned short amount = 0);

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
        equipItem(unsigned short index);

        /**
         * Unequip an item searched by its slot index.
         */
        bool
        unequipItem(unsigned short index);

        /**
         * The function called to use an item applying
         * only the modifiers (for simple items...)
         */
        bool
        use(unsigned short index, BeingPtr itemUser);

        /**
         * The function called to use an item applying
         * only the modifiers (for simple items...)
         */
        bool
        use(unsigned int itemId, BeingPtr itemUser);

        /**
         * The function called to use an item
         * using a script (for complex actions)
         */
        bool
        useWithScript(unsigned short index, const std::string scriptFile);

        /**
         * The function called to use an item
         * using a script (for complex actions)
         */
        bool
        useWithScript(unsigned int itemId, const std::string scriptFile);

    private:
        //Item type
        std::vector<StoredItem> itemList;
};

/**
 * Type definition for a smart pointer to Being.
 */
typedef utils::CountedPtr<Inventory> InventoryPtr;

#endif
