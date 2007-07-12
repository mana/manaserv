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

Inventory::Inventory(Character *p)
  : poss(p->getPossessions()), msg(GPMSG_INVENTORY), client(p)
{}

Inventory::~Inventory()
{
    if (msg.getLength() > 2)
    {
        gameHandler->sendTo(client, msg);
    }
}

void Inventory::sendFull() const
{
    MessageOut m(GPMSG_INVENTORY_FULL);

    for (int i = 0; i < EQUIPMENT_SLOTS; ++i)
    {
        if (int id = poss.equipment[i])
        {
            m.writeByte(i);
            m.writeShort(id);
        }
    }

    int slot = EQUIP_CLIENT_INVENTORY;
    for (std::vector< InventoryItem >::iterator i = poss.inventory.begin(),
         i_end = poss.inventory.end(); i != i_end; ++i)
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

    gameHandler->sendTo(client, m);
}

int Inventory::getItem(int slot) const
{
    for (std::vector< InventoryItem >::iterator i = poss.inventory.begin(),
         i_end = poss.inventory.end(); i != i_end; ++i)
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
    for (std::vector< InventoryItem >::iterator i = poss.inventory.begin(),
         i_end = poss.inventory.end(); i != i_end; ++i, ++index)
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
    for (std::vector< InventoryItem >::iterator i = poss.inventory.begin(),
         i_end = poss.inventory.begin() + index; i != i_end; ++i)
    {
        slot += i->itemId ? 1 : i->amount;
    }
    return slot;
}

int Inventory::fillFreeSlot(int itemId, int amount, int maxPerSlot)
{
    int slot = 0;
    for (int i = 0, i_end = poss.inventory.size(); i < i_end; ++i)
    {
        InventoryItem &it = poss.inventory[i];
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
                poss.inventory.insert(poss.inventory.begin() + i, iu);
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
        poss.inventory.push_back(it);

        msg.writeByte(slot + EQUIP_CLIENT_INVENTORY);
        msg.writeShort(itemId);
        msg.writeByte(nb);
        ++slot;
    }

    return amount;
}

int Inventory::insert(int itemId, int amount)
{
    int slot = 0;
    int maxPerSlot = ItemManager::getItem(itemId)->getMaxPerSlot();

    for (std::vector< InventoryItem >::iterator i = poss.inventory.begin(),
         i_end = poss.inventory.end(); i != i_end; ++i)
    {
        if (i->itemId == itemId)
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

    return amount > 0 ? fillFreeSlot(itemId, amount, maxPerSlot) : 0;
}

int Inventory::count(int itemId) const
{
    int nb = 0;

    for (std::vector< InventoryItem >::iterator i = poss.inventory.begin(),
         i_end = poss.inventory.end(); i != i_end; ++i)
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
    InventoryItem &it = poss.inventory[i];

    if (i == (int)poss.inventory.size() - 1)
    {
        poss.inventory.pop_back();
    }
    else if (poss.inventory[i + 1].itemId == 0)
    {
        it.itemId = 0;
        it.amount = poss.inventory[i + 1].amount + 1;
        poss.inventory.erase(poss.inventory.begin() + i + 1);
    }
    else
    {
        it.itemId = 0;
        it.amount = 1;
    }

    if (i > 0 && poss.inventory[i - 1].itemId == 0)
    {
        // Note: "it" is no longer a valid iterator.
        poss.inventory[i - 1].amount += poss.inventory[i].amount;
        poss.inventory.erase(poss.inventory.begin() + i);
    }
}

int Inventory::remove(int itemId, int amount)
{
    for (int i = poss.inventory.size() - 1; i >= 0; --i)
    {
        InventoryItem &it = poss.inventory[i];
        if (it.itemId == itemId)
        {
            int nb = std::min((int)it.amount, amount);
            it.amount -= nb;
            amount -= nb;

            msg.writeByte(getSlot(i) + EQUIP_CLIENT_INVENTORY);
            msg.writeShort(itemId);
            msg.writeByte(it.amount);

            // If the slot is empty, compress the inventory.
            if (it.amount == 0)
            {
                freeIndex(i);
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
    int i = getIndex(slot);

    if (i < 0)
    {
        return amount;
    }

    InventoryItem &it = poss.inventory[i];
    int nb = std::min((int)it.amount, amount);
    it.amount -= nb;
    amount -= nb;

    msg.writeByte(slot + EQUIP_CLIENT_INVENTORY);
    msg.writeShort(it.itemId);
    msg.writeByte(it.amount);

    // If the slot is empty, compress the inventory.
    if (it.amount == 0)
    {
        freeIndex(i);
    }

    return amount;
}

bool Inventory::equip(int slot)
{
    int itemId = getItem(slot);
    if (!itemId)
    {
        return false;
    }

    int availableSlots = 0, firstSlot = 0, secondSlot = 0;

    switch (ItemManager::getItem(itemId)->getType())
    {
        case ITEM_EQUIPMENT_TWO_HANDS_WEAPON:
        {
            // Special case 1, the two one-handed weapons are to be placed back
            // in the inventory, if there are any.
            int id = poss.equipment[EQUIP_FIGHT1_SLOT];
            if (id && !insert(id, 1))
            {
                return false;
            }

            id = poss.equipment[EQUIP_FIGHT2_SLOT];
            if (id && !insert(id, 1))
            {
                return false;
            }

            msg.writeByte(EQUIP_FIGHT1_SLOT);
            msg.writeShort(itemId);
            msg.writeByte(EQUIP_FIGHT2_SLOT);
            msg.writeShort(0);
            poss.equipment[EQUIP_FIGHT1_SLOT] = itemId;
            poss.equipment[EQUIP_FIGHT2_SLOT] = 0;
            removeFromSlot(slot, 1);
            return true;
        }

        case ITEM_EQUIPMENT_PROJECTILE:
            msg.writeByte(EQUIP_PROJECTILE_SLOT);
            msg.writeShort(itemId);
            poss.equipment[EQUIP_PROJECTILE_SLOT] = itemId;
            return true;

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
            return false;
    }

    int id = poss.equipment[firstSlot];

    switch (availableSlots)
    {
    case 2:
        if (id && !poss.equipment[secondSlot] &&
            ItemManager::getItem(id)->getType() !=
                ITEM_EQUIPMENT_TWO_HANDS_WEAPON)
        {
            // The first slot is full and the second slot is empty.
            msg.writeByte(secondSlot);
            msg.writeShort(itemId);
            poss.equipment[secondSlot] = itemId;
            removeFromSlot(slot, 1);
            return true;
        }
        // no break!

    case 1:
        if (id && !insert(id, 1))
        {
            return false;
        }
        msg.writeByte(firstSlot);
        msg.writeShort(itemId);
        poss.equipment[firstSlot] = itemId;
        removeFromSlot(slot, 1);
        return true;

    default:
        return false;
    }
}
