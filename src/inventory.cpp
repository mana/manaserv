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

#include "inventory.h"
#include "game-server/itemmanager.hpp"

// ---------
// Items
// ---------
unsigned char
Inventory::getInventoryFreeSlot()
{
    for (int a = 0; a < MAX_ITEMS_IN_INVENTORY; a++)
    {
        if (itemList.at(a).amount == 0)
            return a;
    }
    return INVENTORY_FULL;
}

unsigned char
Inventory::getSlotFromId(const unsigned int itemId)
{
    for (int a = 0; a < MAX_ITEMS_IN_INVENTORY; a++)
    {
        if (itemList.at(a).itemId == itemId)
            return a;
    }
    return INVENTORY_FULL;
}

bool
Inventory::hasItem(unsigned int itemId,
                   bool searchInInventory,
                   bool searchInEquipment)
{
    bool hasItem = false;
    // Search in inventory
    if (searchInInventory)
    {
        std::vector<StoredItem>::iterator iter = itemList.begin();
        for (iter = itemList.begin(); iter != itemList.end(); ++iter)
        {
            if (iter->itemId == itemId)
                hasItem = true;
        }
    }
    // Search in equipment
    if (searchInEquipment)
    {
        std::vector<EquippedItem>::iterator equipIter = equippedItemList.begin();
        for (equipIter = equippedItemList.begin();
                equipIter != equippedItemList.end();
                ++equipIter)
        {
            if (equipIter->itemId == itemId)
                hasItem = true;
        }
        if (equippedProjectiles.itemId == itemId)
            hasItem = true;
    }
    return hasItem;
}

short
Inventory::addItem(unsigned int itemId, unsigned char amount)
{
    // We get the max number of item we can have in the same slot
    // for the given item.
    unsigned char maxPerSlot = itemManager->getMaxPerSlot(itemId);
    // We'll add items in slots with the item type and in free slots
    // until it's all done or until the inventory will be all parsed.
    // Searching for items with the same Id in the inventory, before
    // seeking a free slot.
    unsigned char amountToAdd = amount;

    // Parsing inventory
    unsigned char currentAmountInSlot = 0;
    std::vector<StoredItem>::iterator iter = itemList.begin();
    for (iter = itemList.begin(); iter != itemList.end(); ++iter)
    {
        currentAmountInSlot = iter->amount;
        // If a slot has got place for some more of such an item.
        if (iter->itemId == itemId && currentAmountInSlot < maxPerSlot)
        {
            // If there isn't enough space to put every item in the slot.
            // We add the difference.
            if ((maxPerSlot - currentAmountInSlot) < amountToAdd)
            {
                iter->amount += (maxPerSlot - currentAmountInSlot);
                amountToAdd -= (maxPerSlot - currentAmountInSlot);
            }
            else // there is enough to add everything.
            {
                iter->amount += amountToAdd;
                amountToAdd = 0; // Ok!
            }
        }
        // Or if there is an empty slot.
        else if (iter->amount == 0)
        {
            // We add the item in the new slot (setting also the Id.)
            if (maxPerSlot < amountToAdd)
            {
                iter->amount = maxPerSlot;
                amountToAdd -= maxPerSlot;
            }
            else
            {
                iter->amount += amountToAdd;
                amountToAdd = 0; // Ok!
            }
            iter->itemId = itemId;
        }
    }

    return (short)(amount - amountToAdd);
}

short
Inventory::removeItem(unsigned int itemId, unsigned char amount)
{
    // We'll remove items in slots with the item type
    // until it's all done or until the inventory will be all parsed.
    // Searching for items with the same Id in the inventory
    unsigned char amountToRemove = amount;

    // Parsing inventory
    unsigned char currentAmountInSlot = 0;
    std::vector<StoredItem>::iterator iter = itemList.begin();
    for (iter = itemList.begin(); iter != itemList.end(); ++iter)
    {
        currentAmountInSlot = iter->amount;
        // If a slot has got such an item, remove it
        if (iter->itemId == itemId && currentAmountInSlot > 0)
        {
            // If there isn't enough to finish, we remove the difference
            // Emptying the slot, so we clean also the itemId.
            if (currentAmountInSlot <= amountToRemove)
            {
                amountToRemove -= currentAmountInSlot;
                iter->amount = 0;
                iter->itemId = 0;
            }
            else // there is enough to remove everything.
            {
                iter->amount -= amountToRemove;
                amountToRemove = 0; // Ok!
            }
        }
    }
    return (short)(amount - amountToRemove);
}

short
Inventory::removeItem(unsigned char slot, unsigned char amount)
{
    if (itemList.at(slot).amount < amount)
    {
        unsigned char value = itemList.at(slot).amount;
        itemList.at(slot).amount = 0;
        return (short)value;
    }
    else
    {
        itemList.at(slot).amount -= amount;
        return amount;
    }
}

bool
Inventory::use(unsigned char slot, BeingPtr itemUser)
{
    return false; // TODO
}

bool
Inventory::use(unsigned int itemId, BeingPtr itemUser)
{
    return false; // TODO
}

// ---------
// Equipment
// ---------
unsigned char
Inventory::equipItem(unsigned int itemId)
{
    // First, we look for the item in the player's inventory.
    if (!hasItem(itemId, true, false))
        return false;

    unsigned char availableSlots = 0, firstSlot = 0, secondSlot = 0;

    unsigned short itemType = itemManager->getItemType(itemId);
    switch (itemType)
    {
        case ITEM_EQUIPMENT_TWO_HANDS_WEAPON:
        // Special case 1, the two one-handed weapons are to be placed back
        // in the inventory, if there are any.
            if (equippedItemList.at(EQUIP_FIGHT1_SLOT).itemId > 0)
            {   // Slot 1 full
                // old two-handed weapon case:
                if (equippedItemList.at(EQUIP_FIGHT1_SLOT).itemType
                    == ITEM_EQUIPMENT_TWO_HANDS_WEAPON)
                {
                    equippedItemList.at(EQUIP_FIGHT2_SLOT).itemId = 0;
                    equippedItemList.at(EQUIP_FIGHT2_SLOT).itemType = 0;
                }

                if (unequipItem_(equippedItemList.at(EQUIP_FIGHT1_SLOT).itemId,
                EQUIP_FIGHT1_SLOT) == INVENTORY_FULL)
                    return INVENTORY_FULL;
            }
            if (equippedItemList.at(EQUIP_FIGHT2_SLOT).itemId > 0)
            {   // Slot 2 full
                if (unequipItem_(equippedItemList.at(EQUIP_FIGHT2_SLOT).itemId,
                EQUIP_FIGHT2_SLOT) == INVENTORY_FULL)
                    return INVENTORY_FULL;
            }
            // Only the slot 1 needs to be updated.
            return equipItem_(itemId, itemType, EQUIP_FIGHT1_SLOT);
        break;

        case ITEM_EQUIPMENT_PROJECTILE:
        // Special case 2: the projectile is a Stored Item structure,
        // but is still considered as part of the equipment.
            // Case 1: Reloading
            if (equippedProjectiles.itemId == itemId)
            {
                equippedProjectiles.amount += removeItem(itemId,
                            (255 - equippedProjectiles.amount));
                return EQUIP_PROJECTILES_SLOT;
            }
            else // Case 2: Changing projectiles.
            {
                short added;
                added = addItem(equippedProjectiles.itemId,
                                equippedProjectiles.amount);
                if (added == equippedProjectiles.amount)
                { // Ok, we can equip
                    equippedProjectiles.itemId = itemId;
                    equippedProjectiles.amount = removeItem(itemId, 255);
                    return EQUIP_PROJECTILES_SLOT;
                }
                else // Some were unequipped.
                {
                    equippedProjectiles.amount -= added;
                    return INVENTORY_FULL;
                }
            }
        break;

        case ITEM_EQUIPMENT_ONE_HAND_WEAPON:
        case ITEM_EQUIPMENT_SHIELD:
            availableSlots = 2;
            firstSlot = EQUIP_FIGHT1_SLOT;
            secondSlot = EQUIP_FIGHT2_SLOT;
        break;
        case ITEM_EQUIPMENT_RING:
            availableSlots = 2;
            firstSlot = EQUIP_RING1_SLOT;
            secondSlot = EQUIP_RING2_SLOT;
        break;
        case ITEM_EQUIPMENT_BREST:
            availableSlots = 1;
            firstSlot = EQUIP_BREST_SLOT;
        break;
        case ITEM_EQUIPMENT_ARMS:
            availableSlots = 1;
            firstSlot = EQUIP_ARMS_SLOT;
        break;
        case ITEM_EQUIPMENT_HEAD:
            availableSlots = 1;
            firstSlot = EQUIP_HEAD_SLOT;
        break;
        case ITEM_EQUIPMENT_LEGS:
            availableSlots = 1;
            firstSlot = EQUIP_LEGS_SLOT;
        break;
        case ITEM_EQUIPMENT_NECKLACE:
            availableSlots = 1;
            firstSlot = EQUIP_NECKLACE_SLOT;
        break;
        case ITEM_EQUIPMENT_FEET:
            availableSlots = 1;
            firstSlot = EQUIP_FEET_SLOT;
        break;

        case ITEM_UNUSABLE:
        case ITEM_USABLE:
        default:
            return NOT_EQUIPPABLE;
    }

    switch (availableSlots)
    {
    case 1:
        if (equippedItemList.at(firstSlot).itemId > 0)
        {
            if (unequipItem_(equippedItemList.at(firstSlot).itemId,
                firstSlot) != INVENTORY_FULL)
                    return equipItem_(itemId, itemType, firstSlot);
                else
                    return INVENTORY_FULL;
        }
        else // slot empty, we can equip.
        {
            return equipItem_(itemId, itemType, firstSlot);
        }
        break;

    case 2:
        if (equippedItemList.at(firstSlot).itemId > 0)
        {
            // If old weapon is two-handed one, we can unequip
            // the first slot only, and clean the second.
            if (equippedItemList.at(firstSlot).itemId ==
                ITEM_EQUIPMENT_TWO_HANDS_WEAPON)
            {
                if (unequipItem_(equippedItemList.at(firstSlot).itemId,
                    firstSlot) != INVENTORY_FULL)
                    return equipItem_(itemId, itemType, firstSlot);
                else
                    return INVENTORY_FULL;
            }

            if (equippedItemList.at(secondSlot).itemId > 0)
            { // Both slots are full,
                // we remove the first one to equip
                if (unequipItem_(equippedItemList.at(firstSlot).itemId,
                    firstSlot) != INVENTORY_FULL)
                    return equipItem_(itemId, itemType, firstSlot);
                else
                    return INVENTORY_FULL;
            }
            else // Second slot empty, we can equip.
            {
                return equipItem_(itemId, itemType, secondSlot);
            }
        }
        else // first slot empty, we can equip.
        {
            return equipItem_(itemId, itemType, firstSlot);
        }
    break;

    default:
        return NOT_EQUIPPABLE;
    }
}

bool
Inventory::equipItem(unsigned char inventorySlot, unsigned char equipmentSlot)
{
    return false; // TODO
}

bool
Inventory::unequipItem(unsigned int itemId)
{
    return false; // TODO
}

bool
Inventory::unequipItem(unsigned char inventorySlot,
                       unsigned char equipmentSlot)
{
    return false; // TODO
}

unsigned char
Inventory::equipItem_(unsigned int itemId,
                      unsigned int itemType,
                      unsigned char equipmentSlot)
{
    if (removeItem(itemId, 1) == 1)
    {
        equippedItemList.at(equipmentSlot).itemId = itemId;
        equippedItemList.at(equipmentSlot).itemType = itemType;
        return equipmentSlot;
    }
    else
        return NO_ITEM_TO_EQUIP;
}

unsigned char
Inventory::unequipItem_(unsigned int itemId,
             unsigned char equipmentSlot)
{
    if (addItem(itemId, 1) == 1)
    {
        equippedItemList.at(equipmentSlot).itemId = 0;
        equippedItemList.at(equipmentSlot).itemType = 0;
        return equipmentSlot;
    }
    else
        return INVENTORY_FULL;
}
