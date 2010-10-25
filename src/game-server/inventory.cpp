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

#include "game-server/gamehandler.hpp"
#include "game-server/inventory.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "net/messageout.hpp"
#include "utils/logger.h"

// TODO:
// - Inventory::initialise()            Usable but could use a few more things
// - Inventory::equip()                 Usable but last part would be nice

typedef std::set<unsigned int> ItemIdSet;

Inventory::Inventory(Character *p, bool d):
    mPoss(&p->getPossessions()), mInvMsg(GPMSG_INVENTORY),
    mEqmMsg(GPMSG_EQUIP), mClient(p), mDelayed(d)
{
}

Inventory::~Inventory()
{
    commit(false);
}

void Inventory::restart()
{
    mInvMsg.clear();
    mInvMsg.writeShort(GPMSG_INVENTORY);
}

void Inventory::cancel()
{
    assert(mDelayed);
    Possessions &poss = mClient->getPossessions();
    if (mPoss != &poss)
    {
        delete mPoss;
        mPoss = &poss;
    }
    restart();
}

void Inventory::commit(bool doRestart)
{
    Possessions &poss = mClient->getPossessions();
    /* Sends changes, whether delayed or not. */
    if (mInvMsg.getLength() > 2)
    {
        /* Send the message to the client directly. Perhaps this should be
           done through an update flag, too? */
        gameHandler->sendTo(mClient, mInvMsg);
    }
    if (mPoss != &poss)
    {
        if (mDelayed)
        {
            /*
             * Search for any and all changes to equipment.
             * Search through equipment for changes between old and new equipment.
             * Send changes directly when there is a change.
             * Even when equipment references to invy slots are the same, it still
             *      needs to be searched for changes to the internal equiment slot
             *      usage.
             * This is probably the worst part of doing this in delayed mode.
             */
            IdSlotMap oldEquip, newEquip;
            {
                EquipData::const_iterator it1, it2, it1_end, it2_end;
                for (it1 = mPoss->equipSlots.begin(),
                     it1_end = mPoss->equipSlots.end();
                    it1 != it1_end;
                    ++it1)
                {
#ifdef INV_CONST_BOUND_DEBUG
                        IdSlotMap::const_iterator temp2, temp =
#endif
                        newEquip.insert(
                                newEquip.upper_bound(it1->second),
                                std::make_pair(it1->second, it1->first));
#ifdef INV_CONST_BOUND_DEBUG
                        if (temp !=
                            --(temp2 = newEquip.upper_bound(it1->second)))
                            throw;
#endif
                }
                for (it2 = poss.equipSlots.begin(),
                     it2_end = poss.equipSlots.end();
                    it2 != it2_end;
                    ++it2)
                    oldEquip.insert(
                            oldEquip.upper_bound(it2->second),
                            std::make_pair(it2->second, it2->first));
            }
            {
                IdSlotMap::const_iterator it1     = newEquip.begin(),
                                          it2     = oldEquip.begin(),
                                          it1_end = newEquip.end(),
                                          it2_end = oldEquip.end(),
                                          temp1, temp2;
                while (it1 != it1_end || it2 != it2_end)
                {
                    if (it1 == it1_end)
                    {
                        if (it2 == it2_end)
                            break;
                        equip_sub(0, it1);
                    }
                    else if (it2 == it2_end)
                        equip_sub(newEquip.count(it2->first), it2);
                    else if (it1->first == it2->first)
                    {
                        double invSlot = it1->first;
                        while ((it1 != it1_end && it1->first == invSlot) ||
                               (it2 != it2_end && it2->first == invSlot))
                        {
                            /*
                             * Item is still equipped, but need to check
                             *      that the slots didn't change.
                             */
                            if (it1->second == it2->second)
                            {
                                // No change.
                                ++it1;
                                ++it2;
                                continue;
                            }
                            unsigned int itemId =
                                    mPoss->inventory.at(it1->first).itemId;
                            changeEquipment(itemId, itemId);
                            break;
                        }
                    }
                    else if (it1->first > it2->first)
                        equip_sub(newEquip.count(it2->first), it2);
                    else // it1->first < it2->first
                        equip_sub(0, it1);
                }
            }
        }
        poss = *mPoss;
        delete mPoss;
        mPoss = &poss;
    }

    /* Update server sided states if in delayed mode. If we are not in
       delayed mode, the server sided states already reflect the changes
       that have just been sent to the client. */

    if (mEqmMsg.getLength() > 2)
        gameHandler->sendTo(mClient, mEqmMsg);

    if (doRestart)
        restart();
}

void Inventory::equip_sub(unsigned int newCount, IdSlotMap::const_iterator &it)
{
    const unsigned int invSlot = it->first;
    unsigned int count = 0, eqSlot = it->second;
    mEqmMsg.writeShort(invSlot);
    mEqmMsg.writeByte(newCount);
    do {
        if (newCount)
        {
            if (it->second != eqSlot)
            {
                mEqmMsg.writeByte(eqSlot);
                mEqmMsg.writeByte(count);
                count = 1;
                eqSlot = it->second;
            }
            ++count;
        }
        if (itemManager->isEquipSlotVisible(it->second))
            mClient->raiseUpdateFlags(UPDATEFLAG_LOOKSCHANGE);
    } while ((++it)->first == invSlot);
    if (count)
    {
        mEqmMsg.writeByte(eqSlot);
        mEqmMsg.writeByte(count);
    }
    mEqmMsg.writeShort(invSlot);
    changeEquipment(newCount ? 0 : mPoss->inventory.at(invSlot).itemId,
                    newCount ? mPoss->inventory.at(invSlot).itemId : 0);
}

void Inventory::prepare()
{
    if (!mDelayed)
        return;

    Possessions *poss = &mClient->getPossessions();
    if (mPoss == poss)
        mPoss = new Possessions(*poss);
}

void Inventory::sendFull() const
{
    /* Sends all the information needed to construct inventory
       and equipment to the client */
    MessageOut m(GPMSG_INVENTORY_FULL);

    m.writeShort(mPoss->inventory.size());
    for (InventoryData::const_iterator l = mPoss->inventory.begin(),
         l_end = mPoss->inventory.end(); l != l_end; ++l)
    {
        assert(l->second.itemId);
        m.writeShort(l->first); // Slot id
        m.writeShort(l->second.itemId);
        m.writeShort(l->second.amount);
    }

    for (EquipData::const_iterator k = mPoss->equipSlots.begin(),
         k_end = mPoss->equipSlots.end();
         k != k_end;
         ++k)
    {
        m.writeByte(k->first);      // equip slot
        m.writeShort(k->second);    // inventory slot
    }

    gameHandler->sendTo(mClient, m);
}

void Inventory::initialise()
{
    assert(!mDelayed);

    InventoryData::iterator it1;
    EquipData::const_iterator it2, it2_end = mPoss->equipSlots.end();
    /*
     * Apply all exists triggers.
     * Remove unknown inventory items.
     */

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
                item->useTrigger(mClient, ITT_IN_INVY);
            ++it1;
        }
        else
        {
            LOG_WARN("Inventory: deleting unknown item type "
                     << it1->second.itemId << " from the inventory of '"
                     << mClient->getName()
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
        if (equipment.insert(it2->second).second)
        {
            /*
             * Perform checks for equipped items - check that all needed slots are available.
             */
            // TODO - Not needed for testing everything else right now, but
            //        will be needed for production
            /*
             * Apply all equip triggers.
             */
            itemManager->getItem(mPoss->inventory.at(it2->second).itemId)
                    ->useTrigger(mClient, ITT_EQUIP);
        }
    }

    equipment.clear();

    checkSize();
}

void Inventory::checkSize()
{
    /*
     * Check that the inventory size is greater than or equal to the size
     *       needed.
     *       If not, forcibly delete (drop?) items from the end until it is.
     * Check that inventory capacity is greater than or equal to zero.
     *       If not, forcibly delete (drop?) items from the end until it is.
     */
    while (mPoss->inventory.size() > INVENTORY_SLOTS
           || mClient->getModifiedAttribute(ATTR_INV_CAPACITY) < 0) {
        LOG_WARN("Inventory: oversize inventory! Deleting '"
                 << mPoss->inventory.rbegin()->second.amount
                 << "' items of type '"
                 << mPoss->inventory.rbegin()->second.itemId
                 << "' from slot '"
                 << mPoss->inventory.rbegin()->first
                 << "' of character '"
                 << mClient->getName()
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
    unsigned int maxPerSlot = itemManager->getItem(itemId)->getMaxPerSlot();
    if (!itemId || !amount)
        return 0;
    prepare();
    InventoryData::iterator it, it_end = mPoss->inventory.end();
    // Add to slots with existing items of this type first.
    for (it = mPoss->inventory.begin(); it != it_end; ++it)
        if (it->second.itemId == itemId)
        {
            if (it->second.amount >= maxPerSlot)
                continue;
            unsigned short additions = std::min(amount, maxPerSlot)
                                       - it->second.amount;
            amount -= additions;
            it->second.amount += additions;
            mInvMsg.writeShort(it->first);
            mInvMsg.writeShort(itemId);
            mInvMsg.writeShort(it->second.amount);
            if (!amount)
                return 0;
        }

    int slot = 0;
    // We still have some left, so add to blank slots.
    for (it = mPoss->inventory.begin();; ++it)
    {
        if (!amount)
            return 0;
        int lim = it == it_end ? INVENTORY_SLOTS : it->first;
        while (amount && slot < lim)
        {
            int additions = std::min(amount, maxPerSlot);
            mPoss->inventory[slot].itemId = itemId;
            mPoss->inventory[slot].amount = additions;
            amount -= additions;
            mInvMsg.writeShort(slot++); // Last read, so also increment
            mInvMsg.writeShort(itemId);
            mInvMsg.writeShort(additions);
        }
        ++slot; // Skip the slot that the iterator points to
        if (it == it_end) break;
    }

    checkSize();

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
    prepare();
    bool inv = false,
         eq = !itemManager->getItem(itemId)->getItemEquipData().empty();
    for (InventoryData::iterator it = mPoss->inventory.begin(),
                                 it_end = mPoss->inventory.end();
         it != it_end; ++it)
        if (it->second.itemId == itemId)
        {
            if (amount)
            {
                if (eq)
                {
                    // If the item is equippable, we have additional checks to make.
                    bool ch = false;
                    for (EquipData::iterator it2 = mPoss->equipSlots.begin(),
                                         it2_end = mPoss->equipSlots.end();
                         it2 != it2_end;
                         ++it2)
                        if (it2->second == it->first)
                        {
                            if (force)
                                unequip(it2);
                            else
                                ch = inv = true;
                            break;
                        }
                    if (ch && !force)
                        continue;
                }
                unsigned int sub = std::min(amount, it->second.amount);
                amount -= sub;
                it->second.amount -= sub;
                mInvMsg.writeShort(it->first);
                if (it->second.amount)
                {
                    mInvMsg.writeShort(it->second.itemId);
                    mInvMsg.writeShort(it->second.amount);
                    // Some still exist, and we have none left to remove, so
                    // no need to run leave invy triggers.
                    if (!amount)
                        return 0;
                }
                else
                {
                    mInvMsg.writeShort(0);
                    mPoss->inventory.erase(it);
                }
            }
            else
                // We found an instance of them existing and have none left to
                // remove, so no need to run leave invy triggers.
                return 0;
        }
    if (force)
        itemManager->getItem(itemId)->useTrigger(mClient, ITT_LEAVE_INVY);
    // Rather inefficient, but still usable for now assuming small invy size.
    // FIXME
    return inv && !force ? remove(itemId, amount, true) : amount;
}

unsigned int Inventory::move(unsigned int slot1, unsigned int slot2, unsigned int amount)
{
    if (!amount || slot1 == slot2 || slot2 >= INVENTORY_SLOTS)
        return amount;
    prepare();
    InventoryData::iterator it1 = mPoss->inventory.find(slot1),
                            it2 = mPoss->inventory.find(slot2),
                            inv_end = mPoss->inventory.end();

    if (it1 == inv_end)
        return amount;

    EquipData::iterator it, it_end = mPoss->equipSlots.end();
    for (it = mPoss->equipSlots.begin();
         it != it_end;
         ++it)
        if (it->second == slot1)
            // Bad things will happen when you can stack multiple equippable
            //     items in the same slot anyway.
            it->second =  slot2;

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

        mInvMsg.writeShort(slot1);                  // Slot
        if (it1->second.amount)
        {
            mInvMsg.writeShort(it1->second.itemId); // Item Id
            mInvMsg.writeShort(it1->second.amount); // Amount
        }
        else
        {
            mInvMsg.writeShort(0);
            mPoss->inventory.erase(it1);
        }
        mInvMsg.writeShort(slot2);                  // Slot
        mInvMsg.writeShort(it1->second.itemId);     // Item Id (same as slot 1)
        mInvMsg.writeShort(nb);                     // Amount
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

        mInvMsg.writeShort(slot1);                  // Slot
        if (it1->second.amount)
        {
            mInvMsg.writeShort(it1->second.itemId); // Item Id
            mInvMsg.writeShort(it1->second.amount); // Amount
        }
        else
        {
            mInvMsg.writeShort(0);
            mPoss->inventory.erase(it1);
        }
        mInvMsg.writeShort(slot2);                  // Slot
        mInvMsg.writeShort(it2->second.itemId);     // Item Id
        mInvMsg.writeShort(it2->second.amount);     // Amount
    }
    return amount;
}

unsigned int Inventory::removeFromSlot(unsigned int slot, unsigned int amount)
{
    prepare();

    InventoryData::iterator it = mPoss->inventory.find(slot);

    // When the given slot doesn't exist, we can't remove anything
    if (it == mPoss->inventory.end())
        return amount;

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
            ic->useTrigger(mClient, ITT_LEAVE_INVY);
    }

    unsigned int sub = std::min(amount, it->second.amount);
    amount -= sub;
    it->second.amount -= sub;
    mInvMsg.writeShort(it->first);
    if (it->second.amount)
    {
        mInvMsg.writeShort(it->second.itemId);
        mInvMsg.writeShort(it->second.amount);
    }
    else
    {
        mInvMsg.writeShort(0);
        mPoss->inventory.erase(it);
    }
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
        oldI->useTrigger(mClient, ITT_EQUIPCHG);
    else if (oldI)
        oldI->useTrigger(mClient, ITT_UNEQUIP);
    else if (newI)
        newI->useTrigger(mClient, ITT_EQUIP);
}

bool Inventory::equip(int slot, bool override)
{
    if (mPoss->equipSlots.count(slot))
        return false;
    InventoryData::iterator it;
    if ((it = mPoss->inventory.find(slot)) == mPoss->inventory.end())
        return false;
    const ItemEquipsInfo &eq = itemManager->getItem(it->second.itemId)->getItemEquipData();
    if (eq.empty())
        return false;
    ItemEquipInfo const *ovd = 0;
    // Iterate through all possible combinations of slots
    for (ItemEquipsInfo::const_iterator it2 = eq.begin(),
         it2_end = eq.end();
         it2 != it2_end;
         ++it2)
    {
        // Iterate through this combination of slots.
        /*
         * 0 = all ok, slots free
         * 1 = possible if other items are unequipped first
         * 2 = impossible, requires too many slots even with other equipment being removed
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
            if (!mDelayed) {
                mEqmMsg.writeShort(slot);           // Inventory slot
                mEqmMsg.writeByte(it2->size());     // Equip slot type count
            }
            for (it3 = it2->begin(),
                 it3_end = it2->end();
                 it3 != it3_end;
                 ++it3)
            {
                if (!mDelayed) {
                    mEqmMsg.writeByte(it3->first);  // Equip slot
                    mEqmMsg.writeByte(it3->second); // How many are used
                }
                /*
                 * This bit can be somewhat inefficient, but is far better for
                 *  average case assuming most equip use one slot max for each
                 *  type and infrequently (<1/3) two of each type max.
                 * If the reader cares, you're more than welcome to add
                 *  compile time options optimising for other usage.
                 * For now, this is adequate assuming `normal' usage.
                 */
                for (unsigned int i = 0; i < it3->second; ++i)
                    mPoss->equipSlots.insert(
                            std::make_pair(it3->first, slot));
            }
            if (!mDelayed)
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
    // We didn't find a clean equip.
    if (ovd)
    {
        /*
         * We did find an equip that works if we unequip other items, and we can override.
         * Process unequip triggers for all items we have to unequip.
         * Process equip triggers for new item.
         * Attempt to reequip any equipment we had to remove, but disallowing override.
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
    return unequip(it->second, &it);
}

bool Inventory::unequip(unsigned int slot, EquipData::iterator *itp)
{
    prepare();
    EquipData::iterator it = itp ? *itp : mPoss->equipSlots.begin(),
                        it_end = mPoss->equipSlots.end();
    bool changed = false;
    for (it = mPoss->equipSlots.begin();
         it != it_end;
         ++it)
        if (it->second == slot)
        {
            changed = true;
            mPoss->equipSlots.erase(it);
        }
    if (changed && !mDelayed)
    {
        changeEquipment(mPoss->inventory.at(it->second).itemId, 0);
        mEqmMsg.writeShort(slot);
        mEqmMsg.writeByte(0);
    }
    return changed;
}
