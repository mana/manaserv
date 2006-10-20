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
 *  $Id: $
 */

#include "inventory.h"

Inventory::Inventory()
{
    itemList.reserve(MAX_ITEMS_IN_INVENTORY);
    itemList.reserve(TOTAL_EQUIPMENT_SLOTS);
}

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
Inventory::hasItem(unsigned int itemId)
{
    for (int a = 0; a < MAX_ITEMS_IN_INVENTORY; a++)
    {
        if (itemList.at(a).itemId == itemId)
            return true;
    }
    return false;
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
    for (int a = 0; a < MAX_ITEMS_IN_INVENTORY; a++)
    {
        currentAmountInSlot = itemList.at(a).amount;
        // If a slot a got place for some more of such an item.
        if (itemList.at(a).itemId == itemId && currentAmountInSlot < maxPerSlot)
        {
            // If there isn't enough space to put every item in the slot.
            // We add the difference.
            if ((maxPerSlot - currentAmountInSlot) < amountToAdd)
            {
                itemList.at(a).amount += (maxPerSlot - currentAmountInSlot);
                amountToAdd -= (maxPerSlot - currentAmountInSlot);
            }
            else // there is enough to add everything.
            {
                itemList.at(a).amount += amountToAdd;
                amountToAdd = 0; // Ok!
            }
        }
        // Or if there is an empty slot.
        else if (itemList.at(a).amount == 0)
        {
            // We add the item in the new slot (setting also the Id.)
            if (maxPerSlot < amountToAdd)
            {
                itemList.at(a).amount = maxPerSlot;
                amountToAdd -= maxPerSlot;
            }
            else
            {
                itemList.at(a).amount += amountToAdd;
                amountToAdd = 0; // Ok!
            }
            itemList.at(a).itemId = itemId;
        }
    }

    return (short)amount - amountToAdd;
}

short
Inventory::removeItem(unsigned int itemId, unsigned char amount)
{
    return false; // TODO
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
Inventory::equipItem(unsigned int itemId)
{
    return false; // TODO
}

bool
Inventory::unequipItem(unsigned int itemId)
{
    return false; // TODO
}

bool
Inventory::equipItem(unsigned char slot)
{
    return false; // TODO
}

bool
Inventory::unequipItem(unsigned char slot)
{
    return false; // TODO
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
