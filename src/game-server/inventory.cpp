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

#include <algorithm>
#include <cassert>

#include "defines.h"
#include "game-server/gamehandler.hpp"
#include "game-server/inventory.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "net/messageout.hpp"

Inventory::Inventory(Character *p, bool d):
    mPoss(&p->getPossessions()), msg(GPMSG_INVENTORY), mClient(p),
    mDelayed(d), mChangedLook(false)
{
}

Inventory::~Inventory()
{
    if (msg.getLength() > 2)
    {
        update();
        gameHandler->sendTo(mClient, msg);
    }
}

void Inventory::restart()
{
    msg.clear();
    msg.writeShort(GPMSG_INVENTORY);
    mChangedLook = false;
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

void Inventory::update()
{
    if (mDelayed)
    {
        Possessions &poss = mClient->getPossessions();
        if (mPoss != &poss)
        {
            poss = *mPoss;
            delete mPoss;
            mPoss = &poss;
        }
    }
    if (mChangedLook)
    {
        mClient->raiseUpdateFlags(UPDATEFLAG_LOOKSCHANGE);
    }
}

void Inventory::commit()
{
    if (msg.getLength() > 2)
    {
        update();
        gameHandler->sendTo(mClient, msg);
        restart();
    }
}

void Inventory::prepare()
{
    if (!mDelayed)
    {
        return;
    }
    Possessions &poss = mClient->getPossessions();
    if (mPoss == &poss)
    {
        mPoss = new Possessions(poss);
    }
}

void Inventory::sendFull() const
{
    MessageOut m(GPMSG_INVENTORY_FULL);

    for (int i = 0; i < EQUIPMENT_SLOTS; ++i)
    {
        if (int id = mPoss->equipment[i])
        {
            m.writeByte(i);
            m.writeShort(id);
        }
    }

    int slot = EQUIP_CLIENT_INVENTORY;
    for (std::vector< InventoryItem >::const_iterator i = mPoss->inventory.begin(),
         i_end = mPoss->inventory.end(); i != i_end; ++i)
    {
        if (i->itemId)
        {
            m.writeByte(slot);
            m.writeShort(i->itemId);
            m.writeByte(i->amount);
            ++slot;
        }
        else
        {
            slot += i->amount;
        }
    }

    gameHandler->sendTo(mClient, m);
}

int Inventory::getItem(int slot) const
{
    for (std::vector< InventoryItem >::const_iterator i = mPoss->inventory.begin(),
         i_end = mPoss->inventory.end(); i != i_end; ++i)
    {
        if (slot == 0)
        {
            return i->itemId;
        }

        slot -= i->itemId ? 1 : i->amount;

        if (slot < 0)
        {
            return 0;
        }
    }
    return 0;
}

int Inventory::getIndex(int slot) const
{
    int index = 0;
    for (std::vector< InventoryItem >::const_iterator i = mPoss->inventory.begin(),
         i_end = mPoss->inventory.end(); i != i_end; ++i, ++index)
    {
        if (slot == 0)
        {
            return index;
        }

        slot -= i->itemId ? 1 : i->amount;

        if (slot < 0)
        {
            return -1;
        }
    }
    return -1;
}

int Inventory::getSlot(int index) const
{
    int slot = 0;
    for (std::vector< InventoryItem >::const_iterator i = mPoss->inventory.begin(),
         i_end = mPoss->inventory.begin() + index; i != i_end; ++i)
    {
        slot += i->itemId ? 1 : i->amount;
    }
    return slot;
}

int Inventory::fillFreeSlot(int itemId, int amount, int maxPerSlot)
{
    int slot = 0;
    for (int i = 0, i_end = mPoss->inventory.size(); i < i_end; ++i)
    {
        InventoryItem &it = mPoss->inventory[i];
        if (it.itemId == 0)
        {
            int nb = std::min(amount, maxPerSlot);
            if (it.amount <= 1)
            {
                it.itemId = itemId;
                it.amount = nb;
            }
            else
            {
                --it.amount;
                InventoryItem iu = { itemId, nb };
                mPoss->inventory.insert(mPoss->inventory.begin() + i, iu);
                ++i_end;
            }

            msg.writeByte(slot + EQUIP_CLIENT_INVENTORY);
            msg.writeShort(itemId);
            msg.writeByte(nb);

            amount -= nb;
            if (amount == 0)
            {
                return 0;
            }
        }
        ++slot;
    }

    while (slot < INVENTORY_SLOTS - 1 && amount > 0)
    {
        int nb = std::min(amount, maxPerSlot);
        amount -= nb;
        InventoryItem it = { itemId, nb };
        mPoss->inventory.push_back(it);

        msg.writeByte(slot + EQUIP_CLIENT_INVENTORY);
        msg.writeShort(itemId);
        msg.writeByte(nb);
        ++slot;
    }

    return amount;
}

int Inventory::insert(int itemId, int amount)
{
    if (itemId == 0 || amount == 0)
    {
        return 0;
    }

    prepare();

    int maxPerSlot = ItemManager::getItem(itemId)->getMaxPerSlot();
    if (maxPerSlot == 1)
    {
        return fillFreeSlot(itemId, amount, maxPerSlot);
    }

    int slot = 0;
    for (std::vector< InventoryItem >::iterator i = mPoss->inventory.begin(),
         i_end = mPoss->inventory.end(); i != i_end; ++i)
    {
        if (i->itemId == itemId && i->amount < maxPerSlot)
        {
            int nb = std::min(maxPerSlot - i->amount, amount);
            i->amount += nb;
            amount -= nb;

            msg.writeByte(slot + EQUIP_CLIENT_INVENTORY);
            msg.writeShort(itemId);
            msg.writeByte(i->amount);

            if (amount == 0)
            {
                return 0;
            }
            ++slot;
        }
        else
        {
            slot += i->itemId ? 1 : i->amount;
        }
    }

    return fillFreeSlot(itemId, amount, maxPerSlot);
}

int Inventory::count(int itemId) const
{
    int nb = 0;

    for (std::vector< InventoryItem >::const_iterator i = mPoss->inventory.begin(),
         i_end = mPoss->inventory.end(); i != i_end; ++i)
    {
        if (i->itemId == itemId)
        {
            nb += i->amount;
        }
    }

    return nb;
}

void Inventory::freeIndex(int i)
{
    InventoryItem &it = mPoss->inventory[i];

    // Is it the last slot?
    if (i == (int)mPoss->inventory.size() - 1)
    {
        mPoss->inventory.pop_back();
        if (i > 0 && mPoss->inventory[i - 1].itemId == 0)
        {
            mPoss->inventory.pop_back();
        }
        return;
    }

    it.itemId = 0;

    // First concatenate with an empty slot on the right.
    if (mPoss->inventory[i + 1].itemId == 0)
    {
        it.amount = mPoss->inventory[i + 1].amount + 1;
        mPoss->inventory.erase(mPoss->inventory.begin() + i + 1);
    }
    else
    {
        it.amount = 1;
    }

    // Then concatenate with an empty slot on the left.
    if (i > 0 && mPoss->inventory[i - 1].itemId == 0)
    {
        // Note: "it" is no longer a valid reference, hence inventory[i] below.
        mPoss->inventory[i - 1].amount += mPoss->inventory[i].amount;
        mPoss->inventory.erase(mPoss->inventory.begin() + i);
    }
}

int Inventory::remove(int itemId, int amount)
{
    if (itemId == 0 || amount == 0)
    {
        return 0;
    }

    prepare();

    for (int i = mPoss->inventory.size() - 1; i >= 0; --i)
    {
        InventoryItem &it = mPoss->inventory[i];
        if (it.itemId == itemId)
        {
            int nb = std::min((int)it.amount, amount);
            it.amount -= nb;
            amount -= nb;

            msg.writeByte(getSlot(i) + EQUIP_CLIENT_INVENTORY);
            if (it.amount == 0)
            {
                msg.writeShort(0);
                freeIndex(i);
            }
            else
            {
                msg.writeShort(itemId);
                msg.writeByte(it.amount);
            }

            if (amount == 0)
            {
                return 0;
            }
        }
    }

    return amount;
}

int Inventory::removeFromSlot(int slot, int amount)
{
    if (amount == 0)
    {
        return 0;
    }

    int i = getIndex(slot);
    if (i < 0)
    {
        return amount;
    }

    prepare();

    InventoryItem &it = mPoss->inventory[i];
    int nb = std::min((int)it.amount, amount);
    it.amount -= nb;
    amount -= nb;

    msg.writeByte(slot + EQUIP_CLIENT_INVENTORY);
    if (it.amount == 0)
    {
        msg.writeShort(0);
        freeIndex(i);
    }
    else
    {
        msg.writeShort(it.itemId);
        msg.writeByte(it.amount);
    }

    return amount;
}

void Inventory::replaceInSlot(int slot, int itemId, int amount)
{
    int i = getIndex(slot);
    assert(i >= 0);
    prepare();

    msg.writeByte(slot + EQUIP_CLIENT_INVENTORY);
    if (itemId == 0 || amount == 0)
    {
        msg.writeShort(0);
        freeIndex(i);
    }
    else
    {
        InventoryItem &it = mPoss->inventory[i];
        it.itemId = itemId;
        it.amount = amount;
        msg.writeShort(itemId);
        msg.writeByte(amount);
    }
}

void Inventory::equip(int slot)
{
    int itemId = getItem(slot);
    if (!itemId)
    {
        return;
    }

    prepare();

    int availableSlots = 0, firstSlot = 0, secondSlot = 0;

    switch (ItemManager::getItem(itemId)->getType())
    {
        case ITEM_EQUIPMENT_TWO_HANDS_WEAPON:
        {
            // The one-handed weapons are to be placed back in the inventory.
            int id1 = mPoss->equipment[EQUIP_FIGHT1_SLOT],
                id2 = mPoss->equipment[EQUIP_FIGHT2_SLOT];

            if (id2)
            {
                if (id1 && insert(id1, 1) != 0)
                {
                    return;
                }
                id1 = id2;
            }

            replaceInSlot(slot, id1, 1);
            msg.writeByte(EQUIP_FIGHT1_SLOT);
            msg.writeShort(itemId);
            msg.writeByte(EQUIP_FIGHT2_SLOT);
            msg.writeShort(0);
            mPoss->equipment[EQUIP_FIGHT1_SLOT] = itemId;
            mPoss->equipment[EQUIP_FIGHT2_SLOT] = 0;
            mChangedLook = true;
            return;
        }

        case ITEM_EQUIPMENT_PROJECTILE:
            msg.writeByte(EQUIP_PROJECTILE_SLOT);
            msg.writeShort(itemId);
            mPoss->equipment[EQUIP_PROJECTILE_SLOT] = itemId;
            return;

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
        case ITEM_EQUIPMENT_TORSO:
            availableSlots = 1;
            firstSlot = EQUIP_TORSO_SLOT;
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
            return;
    }

    int id = mPoss->equipment[firstSlot];

    if (availableSlots == 2 && id && !mPoss->equipment[secondSlot] &&
        ItemManager::getItem(id)->getType() != ITEM_EQUIPMENT_TWO_HANDS_WEAPON)
    {
        // The first equipment slot is full, but the second one is empty.
        id = 0;
        firstSlot = secondSlot;
    }

    // Put the item in the first equipment slot.
    replaceInSlot(slot, id, 1);
    msg.writeByte(firstSlot);
    msg.writeShort(itemId);
    mPoss->equipment[firstSlot] = itemId;
    mChangedLook = true;
}

void Inventory::unequip(int slot)
{
    int itemId = mPoss->equipment[slot];
    if (!itemId)
    {
        return;
    }
    // No need to prepare.

    if (insert(itemId, 1) == 0)
    {
        msg.writeByte(slot);
        msg.writeShort(0);
        mPoss->equipment[slot] = 0;
        mChangedLook = true;
    }
}
