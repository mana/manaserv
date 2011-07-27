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
        m.writeInt16(k->first);                  // Equip slot id
        m.writeInt16(k->second.itemId);          // Item id
        m.writeInt16(k->second.itemInstance);    // Item instance
    }

    gameHandler->sendTo(mCharacter, m);
}

void Inventory::initialize()
{
    InventoryData::iterator it1;
    EquipData::const_iterator it2, it2_end = mPoss->equipSlots.end();
    /*
     * Apply all exists triggers.
     * Remove unknown inventory items.
     */

    typedef std::set<unsigned int> ItemIdSet;
    ItemIdSet itemIds;

    /*
     * Construct a set of itemIds to keep track of duplicate itemIds.
     */
    for (it1 = mPoss->inventory.begin(); it1 != mPoss->inventory.end();)
    {
        ItemClass *item = itemManager->getItem(it1->second.itemId);
        if (item)
        {
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

    typedef std::set<unsigned int> SlotSet;
    SlotSet equipment;

    /*
     * Construct a set of slot references from equipment to keep track of
     * duplicate slot usage.
     */
    for (it2 = mPoss->equipSlots.begin(); it2 != it2_end; ++it2)
    {
        if (equipment.insert(it2->second.itemInstance).second)
        {
            /*
             * Perform checks for equipped items
             * Check that all needed slots are available.
             */
            // TODO - Not needed for testing everything else right now, but
            //        will be needed for production
            /*
             * Apply all equip triggers.
             */
            itemManager->getItem(it2->second.itemId)
                    ->useTrigger(mCharacter, ITT_EQUIP);
        }
    }

    equipment.clear();

    checkInventorySize();
}

void Inventory::checkInventorySize()
{
    /*
     * Check that the inventory size is greater than or equal to the size
     *       needed.
     *       If not, forcibly delete (drop?) items from the end until it is.
     * Check that inventory capacity is greater than or equal to zero.
     *       If not, forcibly delete (drop?) items from the end until it is.
     */
    while (mPoss->inventory.size() > INVENTORY_SLOTS
           || mCharacter->getModifiedAttribute(ATTR_INV_CAPACITY) < 0)
    {
        LOG_WARN("Inventory: oversize inventory! Deleting '"
                 << mPoss->inventory.rbegin()->second.amount
                 << "' items of type '"
                 << mPoss->inventory.rbegin()->second.itemId
                 << "' from slot '"
                 << mPoss->inventory.rbegin()->first
                 << "' of character '"
                 << mCharacter->getName()
                 << "'!");
        // FIXME Should probably be dropped rather than deleted.
        removeFromSlot(mPoss->inventory.rbegin()->first,
                       mPoss->inventory.rbegin()->second.amount);
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
    InventoryData::iterator it, it_end = mPoss->inventory.end();
    // Add to slots with existing items of this type first.
    for (it = mPoss->inventory.begin(); it != it_end; ++it)
        if (it->second.itemId == itemId)
        {
            // If the slot is full, try the next slot
            if (it->second.amount >= maxPerSlot)
                continue;

            // Add everything that'll fit to the stack
            unsigned short spaceleft = maxPerSlot - it->second.amount;
            if (spaceleft >= amount)
            {
                it->second.amount += amount;
                amount = 0;
            }
            else
            {
                it->second.amount += spaceleft;
                amount -= spaceleft;
            }

            invMsg.writeInt16(it->first);
            invMsg.writeInt16(itemId);
            invMsg.writeInt16(it->second.amount);
            if (!amount)
                break;
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
            invMsg.writeInt16(slot++); // Last read, so also increment
            invMsg.writeInt16(itemId);
            invMsg.writeInt16(additions);
        }
        ++slot; // Skip the slot that the iterator points to
        if (it == it_end) break;
    }

    // Send that first, before checking potential removals
    if (invMsg.getLength() > 2)
        gameHandler->sendTo(mCharacter, invMsg);

    checkInventorySize();

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

unsigned int Inventory::remove(unsigned int itemId, unsigned int amount, bool force)
{
    bool inv = false;

    MessageOut invMsg(GPMSG_INVENTORY);
    bool triggerLeaveInventory = true;
    for (InventoryData::iterator it = mPoss->inventory.begin(),
                                 it_end = mPoss->inventory.end();
         it != it_end; ++it)
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
                }
                else
                {
                    invMsg.writeInt16(0);
                    mPoss->inventory.erase(it);
                }
            }
            else
                // We found an instance of them existing and have none left to
                // remove, so no need to run leave invy triggers.
                triggerLeaveInventory = false;
        }

    if (triggerLeaveInventory)
        itemManager->getItem(itemId)->useTrigger(mCharacter, ITT_LEAVE_INVY);

    if (invMsg.getLength() > 2)
        gameHandler->sendTo(mCharacter, invMsg);

    // Rather inefficient, but still usable for now assuming small invy size.
    // FIXME
    return inv && !force ? remove(itemId, amount, true) : amount;
}

unsigned int Inventory::move(unsigned int slot1, unsigned int slot2,
                             unsigned int amount)
{
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
        invMsg.writeInt16(it1->second.itemId);     // Item Id (same as slot 1)
        invMsg.writeInt16(nb);                     // Amount
    }
    else
    {
        // Slot2 exists.
        if (it2->second.itemId != it1->second.itemId)
            return amount; // Cannot stack items of a different type.
        nb = std::min(itemManager->getItem(it1->second.itemId)->getMaxPerSlot()
                          - it2->second.amount,
                      nb);

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

    MessageOut invMsg(GPMSG_INVENTORY);
    // Check if an item of the same class exists elsewhere in the inventory
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
    if (!exists && it->second.itemId) {
        if (ItemClass *ic = itemManager->getItem(it->second.itemId))
            ic->useTrigger(mCharacter, ITT_LEAVE_INVY);
    }

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
        mPoss->inventory.erase(it);
    }

    if (invMsg.getLength() > 2)
        gameHandler->sendTo(mCharacter, invMsg);

    return amount;
}


void Inventory::changeEquipment(unsigned int oldId, unsigned int newId)
{
    if (!oldId && !newId)
        return;
    changeEquipment(oldId ? itemManager->getItem(oldId) : 0,
                    newId ? itemManager->getItem(newId) : 0);
}

void Inventory::changeEquipment(ItemClass *oldI, ItemClass *newI)
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

bool Inventory::equip(int slot, bool override)
{
    if (mPoss->equipSlots.count(slot))
        return false;
    InventoryData::iterator it;
    if ((it = mPoss->inventory.find(slot)) == mPoss->inventory.end())
        return false;
    const ItemEquipsInfo &eq = itemManager->getItem(it->second.itemId)
                                    ->getItemEquipData();
    if (eq.empty())
        return false;
    ItemEquipInfo const *ovd = 0;

    MessageOut equipMsg(GPMSG_EQUIP);
    // Iterate through all possible combinations of slots
    for (ItemEquipsInfo::const_iterator it2 = eq.begin(),
         it2_end = eq.end(); it2 != it2_end; ++it2)
    {
        // Iterate through this combination of slots.
        /*
         * 0 = all ok, slots free
         * 1 = possible if other items are unequipped first
         * 2 = impossible, requires too many slots
         *     even with other equipment being removed
         */
        int fail = 0;
        ItemEquipInfo::const_iterator it3, it3_end;
        for (it3 = it2->begin(),
             it3_end = it2->end();
             it3 != it3_end;
             ++it3)
        {
            // it3 -> { slot id, number required }
            unsigned int max = itemManager->getMaxSlotsFromId(it3->first),
                         used = mPoss->equipSlots.count(it3->first);
            if (max - used >= it3->second)
                continue;
            else if (max >= it3->second)
            {
                fail |= 1;
                if (override)
                    continue;
                else
                    break;
            }
            else
            {
                fail |= 2;
                break;
            }
        }
        switch (fail)
        {
            case 0:
            /*
             * Clean fit. Equip and apply immediately.
             */
            equipMsg.writeInt16(slot);           // Inventory slot
            equipMsg.writeInt8(it2->size());     // Equip slot type count
            for (it3 = it2->begin(),
                 it3_end = it2->end();
                 it3 != it3_end;
                 ++it3)
            {
                equipMsg.writeInt8(it3->first);  // Equip slot
                equipMsg.writeInt8(it3->second); // How many are used
                /*
                 * This bit can be somewhat inefficient, but is far better for
                 *  average case assuming most equip use one slot max for each
                 *  type and infrequently (<1/3) two of each type max.
                 * If the reader cares, you're more than welcome to add
                 *  compile time options optimising for other usage.
                 * For now, this is adequate assuming `normal' usage.
                 */
                /** Part disabled until reimplemented*/
                /*for (unsigned int i = 0; i < it3->second; ++i)
                    mPoss->equipSlots.insert(
                            std::make_pair(it3->first, slot));*/
            }

            changeEquipment(0, it->second.itemId);
            return true;
            case 1:
            /*
             * Definitions earlier in the item file have precedence (even if it
             *      means requiring unequipping more), so no need to store more
             *      than the first.
             */
            if (override && !ovd)
                ovd = &*it2; // Iterator -> object -> pointer.
            break;
            case 2:
            default:
            /*
             * Since slots are currently static (and I don't see any reason to
             *      change this right now), something probably went wrong.
             * The logic to catch this is here rather than in the item manager
             *      just in case non-static equip slots do want to be
             *      implemented later. This would not be a trivial task,
             *      however.
             */
            LOG_WARN("Inventory - item '" << it->second.itemId <<
                     "' cannot be equipped, even by unequipping other items!");
            break;
        }
    }

    if (equipMsg.getLength() > 2)
        gameHandler->sendTo(mCharacter, equipMsg);
    // We didn't find a clean equip.
    if (ovd)
    {
        /*
         * We did find an equip that works if we unequip other items,
         * and we can override.
         * Process unequip triggers for all items we have to unequip.
         * Process equip triggers for new item.
         * Attempt to reequip any equipment we had to remove,
         * but disallowing override.
         */

        // TODO - this would increase ease of use substatially, add as soon as
        // there is time to do so.

        return false; // Return true when this section is complete
    }
    /*
     * We cannot equip, either because we could not find any valid equip process
     *     or because we found a dirty equip and weren't allowed to override.
     */
    return false;
}

bool Inventory::unequip(EquipData::iterator it)
{
    return unequip(it->first, &it);
}

bool Inventory::unequip(unsigned int slot, EquipData::iterator *itp)
{
    EquipData::iterator it = itp ? *itp : mPoss->equipSlots.begin(),
                        it_end = mPoss->equipSlots.end();
    bool changed = false;

    MessageOut equipMsg(GPMSG_EQUIP);
    // Erase all equip entries that point to the given inventory slot
    while (it != it_end)
    {
        if (it->first == slot)
        {
            changed = true;
            mPoss->equipSlots.erase(it++);
        }
        else
        {
            ++it;
        }
    }

    if (changed)
    {
        changeEquipment(mPoss->inventory.at(slot).itemId, 0);
        equipMsg.writeInt16(slot);
        equipMsg.writeInt8(0);
    }

    if (equipMsg.getLength() > 2)
        gameHandler->sendTo(mCharacter, equipMsg);

    return changed;
}
