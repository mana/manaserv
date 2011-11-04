/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
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

#include <algorithm>
#include <cassert>

#include "game-server/gamehandler.h"
#include "game-server/inventory.h"
#include "game-server/item.h"
#include "game-server/itemmanager.h"
#include "game-server/state.h"
#include "net/messageout.h"
#include "utils/logger.h"

Inventory::Inventory(Character *p):
    mPoss(&p->getPossessions()), mCharacter(p)
{
}

void Inventory::sendFull() const
{
    /* Sends all the information needed to construct inventory
       and equipment to the client */
    MessageOut m(GPMSG_INVENTORY_FULL);

    m.writeInt16(mPoss->inventory.size());
    for (InventoryData::const_iterator l = mPoss->inventory.begin(),
         l_end = mPoss->inventory.end(); l != l_end; ++l)
    {
        assert(l->second.itemId);
        m.writeInt16(l->first); // Slot id
        m.writeInt16(l->second.itemId);
        m.writeInt16(l->second.amount);
    }

    for (EquipData::const_iterator k = mPoss->equipSlots.begin(),
         k_end = mPoss->equipSlots.end();
         k != k_end;
         ++k)
    {
        m.writeInt16(k->first);                 // Equip slot id
        m.writeInt16(k->second.itemId);         // Item id
        m.writeInt16(k->second.itemInstance);   // Item instance
    }

    gameHandler->sendTo(mCharacter, m);
}

void Inventory::initialize()
{
    /*
     * Construct a set of item Ids to keep track of duplicate item Ids.
     */
    std::set<unsigned int> itemIds;

    /*
     * Construct a set of itemIds to keep track of duplicate itemIds.
     */
    InventoryData::iterator it1;
    for (it1 = mPoss->inventory.begin(); it1 != mPoss->inventory.end();)
    {
        ItemClass *item = itemManager->getItem(it1->second.itemId);
        if (item)
        {
            // If the insertion succeeded, it's the first time we're
            // adding the item in the inventory. Hence, we can trigger
            // item presence in inventory effect.
            if (itemIds.insert(it1->second.itemId).second)
                item->useTrigger(mCharacter, ITT_IN_INVY);
            ++it1;
        }
        else
        {
            LOG_WARN("Inventory: deleting unknown item type "
                     << it1->second.itemId << " from the inventory of '"
                     << mCharacter->getName()
                     << "'!");
            mPoss->inventory.erase(it1++);
        }
    }

    itemIds.clear();

    /*
     * Equipment effects can be cumulative if more than one item instance
     * is equipped, but we check to trigger the item presence in equipment
     * effect only based on the first item instance insertion.
     */
    EquipData::iterator it2;
    for (it2 = mPoss->equipSlots.begin(); it2 != mPoss->equipSlots.end();)
    {
        ItemClass *item = itemManager->getItem(it2->second.itemId);
        if (item)
        {
            // TODO: Check equip conditions.
            // If not all needed slots are there, put the item back
            // in the inventory.
        }
        else
        {
            LOG_WARN("Equipment: deleting unknown item id "
                     << it2->second.itemId << " from the equipment of '"
                     << mCharacter->getName()
                     << "'!");
            mPoss->equipSlots.erase(it2++);
            continue;
        }

        /*
         * Apply all equip triggers at first item instance insertion
         */
        if (itemIds.insert(it2->second.itemInstance).second)
        {
            itemManager->getItem(it2->second.itemId)
                ->useTrigger(mCharacter, ITT_EQUIP);
        }

        ++it2;
    }
}

unsigned int Inventory::getItem(unsigned int slot) const
{
    InventoryData::iterator item = mPoss->inventory.find(slot);
    return item != mPoss->inventory.end() ? item->second.itemId : 0;
}

unsigned int Inventory::insert(unsigned int itemId, unsigned int amount)
{
    if (!itemId || !amount)
        return 0;

    MessageOut invMsg(GPMSG_INVENTORY);
    unsigned int maxPerSlot = itemManager->getItem(itemId)->getMaxPerSlot();

    LOG_DEBUG("Inventory: Inserting " << amount << " item(s) Id: " << itemId
              << " for character '" << mCharacter->getName() << "'.");

    InventoryData::iterator it, it_end = mPoss->inventory.end();
    // Add to slots with existing items of this type first.
    for (it = mPoss->inventory.begin(); it != it_end; ++it)
    {
        if (it->second.itemId == itemId)
        {
            // If the slot is full, try the next slot
            if (it->second.amount >= maxPerSlot)
                continue;

            // Add everything that'll fit to the stack
            unsigned short spaceLeft = maxPerSlot - it->second.amount;
            if (spaceLeft >= amount)
            {
                it->second.amount += amount;
                amount = 0;
                LOG_DEBUG("Everything inserted at slot id: " << it->first);
            }
            else
            {
                it->second.amount += spaceLeft;
                amount -= spaceLeft;
                LOG_DEBUG(spaceLeft << " item(s) inserted at slot id: "
                          << it->first);
            }

            invMsg.writeInt16(it->first);
            invMsg.writeInt16(itemId);
            invMsg.writeInt16(it->second.amount);
            if (!amount)
                break;
        }
    }

    int slot = 0;
    // We still have some left, so add to blank slots.
    for (it = mPoss->inventory.begin();; ++it)
    {
        if (!amount)
            break;
        int lim = (it == it_end) ? INVENTORY_SLOTS : it->first;
        while (amount && slot < lim)
        {
            int additions = std::min(amount, maxPerSlot);
            mPoss->inventory[slot].itemId = itemId;
            mPoss->inventory[slot].amount = additions;
            amount -= additions;
            LOG_DEBUG(additions << " item(s) inserted at slot id: " << slot);
            invMsg.writeInt16(slot++); // Last read, so also increment
            invMsg.writeInt16(itemId);
            invMsg.writeInt16(additions);
        }
        ++slot; // Skip the slot that the iterator points to
        if (it == it_end)
            break;
    }

    // Send that first, before checking potential removals
    if (invMsg.getLength() > 2)
        gameHandler->sendTo(mCharacter, invMsg);

    return amount;
}

unsigned int Inventory::count(unsigned int itemId) const
{
    unsigned int nb = 0;
    for (InventoryData::iterator it = mPoss->inventory.begin(),
                                 it_end = mPoss->inventory.end();
         it != it_end; ++it)
        if (it->second.itemId == itemId)
            nb += it->second.amount;
    return nb;
}

unsigned int Inventory::remove(unsigned int itemId, unsigned int amount)
{
    if (!itemId || !amount)
        return amount;

    LOG_DEBUG("Inventory: Request remove of " << amount << " item(s) id: "
              << itemId << " for character: '" << mCharacter->getName()
              << "'.");

    MessageOut invMsg(GPMSG_INVENTORY);
    bool triggerLeaveInventory = true;
    for (InventoryData::iterator it = mPoss->inventory.begin(),
             it_end = mPoss->inventory.end(); it != it_end; ++it)
    {
        if (it->second.itemId == itemId)
        {
            if (amount)
            {
                unsigned int sub = std::min(amount, it->second.amount);
                amount -= sub;
                it->second.amount -= sub;
                invMsg.writeInt16(it->first);
                if (it->second.amount)
                {
                    invMsg.writeInt16(it->second.itemId);
                    invMsg.writeInt16(it->second.amount);
                    // Some still exist, and we have none left to remove, so
                    // no need to run leave invy triggers.
                    if (!amount)
                        triggerLeaveInventory = false;
                    LOG_DEBUG("Slot id: " << it->first << " has now "
                              << it->second.amount << "item(s).");
                }
                else
                {
                    invMsg.writeInt16(0);
                    mPoss->inventory.erase(it);
                    LOG_DEBUG("Slot id: " << it->first << " is now empty.");
                }
            }
            else
            {
                // We found an instance of them existing and have none left to
                // remove, so no need to run leave invy triggers.
                triggerLeaveInventory = false;
            }
        }
    }

    if (triggerLeaveInventory)
        itemManager->getItem(itemId)->useTrigger(mCharacter, ITT_LEAVE_INVY);

    if (invMsg.getLength() > 2)
        gameHandler->sendTo(mCharacter, invMsg);

    return amount;
}

unsigned int Inventory::move(unsigned int slot1, unsigned int slot2,
                             unsigned int amount)
{
    LOG_DEBUG(amount << " item(s) requested to move from: " << slot1 << " to "
              << slot2 << " for character: '" << mCharacter->getName() << "'.");

    if (!amount || slot1 == slot2 || slot2 >= INVENTORY_SLOTS)
        return amount;

    InventoryData::iterator it1 = mPoss->inventory.find(slot1),
                            it2 = mPoss->inventory.find(slot2),
                            inv_end = mPoss->inventory.end();

    if (it1 == inv_end)
        return amount;

    MessageOut invMsg(GPMSG_INVENTORY);

    unsigned int nb = std::min(amount, it1->second.amount);
    if (it2 == inv_end)
    {
        // Slot2 does not yet exist.
        mPoss->inventory[slot2].itemId = it1->second.itemId;
        nb = std::min(itemManager->getItem(it1->second.itemId)->getMaxPerSlot(),
                      nb);

        mPoss->inventory[slot2].amount = nb;
        it1->second.amount -= nb;
        amount -= nb;

        //Save the itemId in case of deletion of the iterator
        unsigned int itemId = it1->second.itemId;
        invMsg.writeInt16(slot1);                  // Slot
        if (it1->second.amount)
        {
            invMsg.writeInt16(it1->second.itemId); // Item Id
            invMsg.writeInt16(it1->second.amount); // Amount
            LOG_DEBUG("Left " << amount << " item(s) id:"
                      << it1->second.itemId << " into slot: " << slot1);
        }
        else
        {
            invMsg.writeInt16(0);
            mPoss->inventory.erase(it1);
            LOG_DEBUG("Slot: " << slot1 << " is now empty.");
        }
        invMsg.writeInt16(slot2);                  // Slot
        invMsg.writeInt16(itemId);     // Item Id (same as slot 1)
        invMsg.writeInt16(nb);                     // Amount
        LOG_DEBUG("Slot: " << slot2 << " has now " << nb << " of item id: "
                  << itemId);
    }
    else
    {
        // Slot2 exists.
        if (it2->second.itemId != it1->second.itemId)
         {
            // Swap items when they are of a different type
            // and when all the amount of slot 1 is moving onto slot 2.
            if (amount >= it1->second.amount)
            {
                unsigned int itemId = it1->second.itemId;
                unsigned int amount = it1->second.amount;
                it1->second.itemId = it2->second.itemId;
                it1->second.amount = it2->second.amount;
                it2->second.itemId = itemId;
                it2->second.amount = amount;

                // Sending swapped slots.
                invMsg.writeInt16(slot1);
                invMsg.writeInt16(it1->second.itemId);
                invMsg.writeInt16(it1->second.amount);
                invMsg.writeInt16(slot2);
                invMsg.writeInt16(it2->second.itemId);
                invMsg.writeInt16(it2->second.amount);
                LOG_DEBUG("Swapping items in slots " << slot1
                          << " and " << slot2);
            }
            else
            {
                // Cannot partially stack items of a different type.
                LOG_DEBUG("Cannot move " << amount << " item(s) from slot "
                          << slot1 << " to " << slot2);
                return amount;
            }
        }
        else // Same item type on slot 2.
        {
            // Number of items moving
            nb = std::min(itemManager->getItem(
                              it1->second.itemId)->getMaxPerSlot()
                              - it2->second.amount, nb);

            // If nothing can move, we can abort
            if (!nb)
                return amount;

            it1->second.amount -= nb;
            it2->second.amount += nb;
            amount -= nb;

            invMsg.writeInt16(slot1);                  // Slot
            if (it1->second.amount)
            {
                invMsg.writeInt16(it1->second.itemId); // Item Id
                invMsg.writeInt16(it1->second.amount); // Amount
            }
            else
            {
                invMsg.writeInt16(0);
                mPoss->inventory.erase(it1);
            }
            invMsg.writeInt16(slot2);                  // Slot
            invMsg.writeInt16(it2->second.itemId);     // Item Id
            invMsg.writeInt16(it2->second.amount);     // Amount
        }
    }

    if (invMsg.getLength() > 2)
        gameHandler->sendTo(mCharacter, invMsg);

    return amount;
}

unsigned int Inventory::removeFromSlot(unsigned int slot, unsigned int amount)
{
    InventoryData::iterator it = mPoss->inventory.find(slot);

    // When the given slot doesn't exist, we can't remove anything
    if (it == mPoss->inventory.end())
        return amount;

    LOG_DEBUG("Inventory: Request Removal of " << amount << " item(s) in slot: "
              << slot << " for character: '" << mCharacter->getName() << "'.");

    MessageOut invMsg(GPMSG_INVENTORY);
    // Check if an item of the same id exists elsewhere in the inventory
    bool exists = false;
    for (InventoryData::const_iterator it2 = mPoss->inventory.begin(),
         it2_end = mPoss->inventory.end();
         it2 != it2_end; ++it2)
    {
        if (it2->second.itemId == it->second.itemId
                && it->first != it2->first)
        {
            exists = true;
            break;
        }
    }

    // We check whether it's the last slot where we can find that item id.
    bool lastSlotOfItemRemaining = false;
    if (!exists && it->second.itemId)
        lastSlotOfItemRemaining = true;

    unsigned int sub = std::min(amount, it->second.amount);
    amount -= sub;
    it->second.amount -= sub;
    invMsg.writeInt16(it->first);
    if (it->second.amount)
    {
        invMsg.writeInt16(it->second.itemId);
        invMsg.writeInt16(it->second.amount);
    }
    else
    {
        invMsg.writeInt16(0);

        // The item(s) was(were) the last one(s) in the inventory.
        if (lastSlotOfItemRemaining)
        {
            if (ItemClass *ic = itemManager->getItem(it->second.itemId))
                ic->useTrigger(mCharacter, ITT_LEAVE_INVY);
        }
        mPoss->inventory.erase(it);
    }

    if (invMsg.getLength() > 2)
        gameHandler->sendTo(mCharacter, invMsg);

    return amount;
}


void Inventory::updateEquipmentTrigger(unsigned int oldId, unsigned int newId)
{
    if (!oldId && !newId)
        return;
    updateEquipmentTrigger(oldId ? itemManager->getItem(oldId) : 0,
                    newId ? itemManager->getItem(newId) : 0);
}

void Inventory::updateEquipmentTrigger(ItemClass *oldI, ItemClass *newI)
{
    // This should only be called when applying changes, either directly
    // in non-delayed mode or when the changes are committed in delayed mode.
    if (!oldI && !newI)
        return;
    if (oldI && newI)
        oldI->useTrigger(mCharacter, ITT_EQUIPCHG);
    else if (oldI)
        oldI->useTrigger(mCharacter, ITT_UNEQUIP);
    else if (newI)
        newI->useTrigger(mCharacter, ITT_EQUIP);
}

unsigned int Inventory::getNewEquipItemInstance()
{
    unsigned int itemInstance = 1;

    for (EquipData::const_iterator it = mPoss->equipSlots.begin(),
        it_end = mPoss->equipSlots.end(); it != it_end; ++it)
    {
        if (it->second.itemInstance == itemInstance)
        {
            ++itemInstance;
            it = mPoss->equipSlots.begin();
        }
    }

    return itemInstance;
}

bool Inventory::checkEquipmentCapacity(unsigned int equipmentSlot,
                                       unsigned int capacityRequested)
{
    int capacity = itemManager->getEquipSlotCapacity(equipmentSlot);

    // If the equipement slot doesn't exist, we can't equip on it.
    if (capacity <= 0)
        return false;

    // Test whether the slot capacity requested is reached.
    for (EquipData::const_iterator it = mPoss->equipSlots.begin(),
        it_end = mPoss->equipSlots.end(); it != it_end; ++it)
    {
        if (it->first == equipmentSlot)
        {
            if (it->second.itemInstance != 0)
            {
                capacity--;
            }
        }
    }

    assert(capacity >= 0); // A should never happen case.

    if (capacity < (int)capacityRequested)
        return false;

    return true;
}

bool Inventory::equip(int inventorySlot)
{
    // Test inventory slot existence
    InventoryData::iterator it;
    if ((it = mPoss->inventory.find(inventorySlot)) == mPoss->inventory.end())
    {
        return false;
        LOG_DEBUG("No existing item in inventory at slot: " << inventorySlot);
    }

    // Test the equipment scripted requirements
    if (!testEquipScriptRequirements(it->second.itemId))
        return false;

    // Test the equip requirements. If none, it's not an equipable item.
    const ItemEquipRequirement &equipReq =
        itemManager->getItem(it->second.itemId)->getItemEquipRequirement();
    if (!equipReq.equipSlotId)
    {
        LOG_DEBUG("No equip requirements for item id: " << it->second.itemId
            << " at slot: " << inventorySlot);
        return false;
    }

    // List of potential unique itemInstances to unequip first.
    std::set<unsigned int> equipInstancesToUnequipFirst;

    // We first check the equipment slots for:
    // - 1. whether enough total equip slot space is available.
    // - 2. whether some other equipment is to be unequipped first.

    // If not enough total space in the equipment slot is available,
    // we cannot equip.
    if (itemManager->getEquipSlotCapacity(equipReq.equipSlotId)
            < equipReq.capacityRequired)
    {
        LOG_DEBUG("Not enough equip capacity at slot: " << equipReq.equipSlotId
                  << ", total available: "
                  << itemManager->getEquipSlotCapacity(equipReq.equipSlotId)
                  << ", required: " << equipReq.capacityRequired);
        return false;
    }

    // Test whether some item(s) is(are) to be unequipped first.
    if (!checkEquipmentCapacity(equipReq.equipSlotId,
                                equipReq.capacityRequired))
    {
        // And test whether the unequip action would succeed first.
        if (testUnequipScriptRequirements(equipReq.equipSlotId)
            && hasInventoryEnoughSpace(equipReq.equipSlotId))
        {
            // Then, we unequip each iteminstance of the equip slot
            for (EquipData::iterator iter =
                mPoss->equipSlots.begin();
                iter != mPoss->equipSlots.end(); ++iter)
            {
                if (iter->first == equipReq.equipSlotId
                    && iter->second.itemInstance)
                    equipInstancesToUnequipFirst.insert(
                                                     iter->second.itemInstance);
            }
        }
        else
        {
            // Some non-unequippable equipment is to be unequipped first.
            // Can be the case of cursed items,
            // or when the inventory is full, for instance.
            return false;
        }
    }

    // Potential Pre-unequipment process
    for (std::set<unsigned int>::const_iterator it3 =
            equipInstancesToUnequipFirst.begin();
            it3 != equipInstancesToUnequipFirst.end(); ++it3)
    {
        if (!unequip(*it3))
        {
            // Something went wrong even when we tested the unequipment process.
            LOG_WARN("Unable to unequip even when unequip was tested. "
                     "Character : " << mCharacter->getName()
                     << ", unequip slot: " << *it3);
            return false;
        }
    }

    // Actually equip the item now that the requirements has met.
    //W equip slot type count, W item id, { W equip slot, W capacity used}*
    MessageOut equipMsg(GPMSG_EQUIP);
    equipMsg.writeInt16(it->second.itemId); // Item Id
    equipMsg.writeInt16(1); // Number of equip slot changed.

    // Compute an unique equip item Instance id (unicity is per character only.)
    int itemInstance = getNewEquipItemInstance();

    unsigned int capacityLeft = equipReq.capacityRequired;
    unsigned int capacityUsed = 0;
    // Apply equipment changes
    for (EquipData::iterator it4 = mPoss->equipSlots.begin(),
         it4_end = mPoss->equipSlots.end(); it4 != it4_end; ++it4)
    {
        if (!capacityLeft)
            break;

        // We've found an existing equip slot
        if (it4->first == equipReq.equipSlotId)
        {
            // We've found an empty slot
            if (it4->second.itemInstance == 0)
            {
                it4->second.itemId = it->second.itemId;
                it4->second.itemInstance = itemInstance;
                --capacityLeft;
            }
            else // The slot is already in use.
            {
                ++capacityUsed;
            }
        }
    }

    // When there is still something to apply even when out of that loop,
    // It means that the equip multimapis missing empty slots.
    // Hence, we add them back
    if(capacityLeft)
    {
        unsigned int maxCapacity =
            itemManager->getEquipSlotCapacity(equipReq.equipSlotId);

        // A should never happen case
        assert(maxCapacity >= capacityUsed + capacityLeft);

        while (capacityLeft)
        {
            EquipmentItem equipItem(it->second.itemId, itemInstance);
            mPoss->equipSlots.insert(
                std::make_pair<unsigned int, EquipmentItem>
                    (equipReq.equipSlotId, equipItem));
            --capacityLeft;
        }
    }

    // Equip slot
    equipMsg.writeInt16(equipReq.equipSlotId);
    // Capacity used
    equipMsg.writeInt16(equipReq.capacityRequired);
    // Item instance
    equipMsg.writeInt16(itemInstance);

    // New item trigger
    updateEquipmentTrigger(0, it->second.itemId);

    // Remove item from inventory
    removeFromSlot(inventorySlot, 1);

    gameHandler->sendTo(mCharacter, equipMsg);

    // Update look when necessary
    checkLookchanges(equipReq.equipSlotId);

    return true;
}

bool Inventory::unequip(unsigned int itemInstance)
{
    if (!itemInstance)
        return false;

    // The itemId to unequip
    unsigned int itemId = 0;
    unsigned int slotTypeId = 0;

    bool addedToInventory = false;

    // Empties all equip entries that point to the given equipment slot
    // The equipment slots should NEVER be erased after initialization!
    for (EquipData::iterator it = mPoss->equipSlots.begin(),
            it_end = mPoss->equipSlots.end(); it != it_end; ++it)
    {
        if (it->second.itemInstance == itemInstance && it->second.itemId)
        {
            // Add the item to the inventory list if not already present there
            itemId = it->second.itemId;

            // Move the item back to inventory and return false when it failed.
            if (!addedToInventory && insert(itemId, 1) > 0)
                return false;
            else
                addedToInventory = true;

            it->second.itemId = 0;
            it->second.itemInstance = 0;

            // We keep track of the slot type to be able to raise a potential
            // change in the character sprite
            slotTypeId = it->first;
        }
    }

    // When there were no corresponding item id, it means no item was to
    // be unequipped.
    if (!itemId)
        return false;

    MessageOut equipMsg(GPMSG_EQUIP);
    equipMsg.writeInt16(0); // Item Id, useless in case of unequip.
    equipMsg.writeInt16(1); // Number of slot types touched.
    equipMsg.writeInt16(itemInstance);
    equipMsg.writeInt16(0); // Capacity used, set to 0 to unequip.
    gameHandler->sendTo(mCharacter, equipMsg);

    // Apply unequip trigger
    updateEquipmentTrigger(itemId, 0);

    checkLookchanges(slotTypeId);

    return true;
}

void Inventory::checkLookchanges(unsigned int slotTypeId)
{
    if (itemManager->isEquipSlotVisible(slotTypeId))
        mCharacter->raiseUpdateFlags(UPDATEFLAG_LOOKSCHANGE);
}
