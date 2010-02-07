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

    m.writeByte(255);
    m.writeLong(mPoss->money);

    gameHandler->sendTo(mClient, m);
}

void Inventory::initialize()
{
    assert(!mDelayed);

    // First, check the equipment and apply its modifiers.
    for (int i = 0; i < EQUIP_PROJECTILE_SLOT; ++i)
    {
        int itemId = mPoss->equipment[i];
        if (!itemId) continue;
        if (ItemClass *ic = ItemManager::getItem(itemId))
        {
            ic->getModifiers().applyAttributes(mClient);
        }
        else
        {
            mPoss->equipment[i] = 0;
            LOG_WARN("Removed unknown item " << itemId << " from equipment "
                     "of character " << mClient->getDatabaseID() << '.');
        }
    }

    // Second, remove unknown inventory items.
    int i = 0;
    while (i < (int)mPoss->inventory.size())
    {
        int itemId = mPoss->inventory[i].itemId;
        if (itemId)
        {
            ItemClass *ic = ItemManager::getItem(itemId);
            if (!ic)
            {
                LOG_WARN("Removed unknown item " << itemId << " from inventory"
                         " of character " << mClient->getDatabaseID() << '.');
                freeIndex(i);
                continue;
            }
        }
        ++i;
    }
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
            return i->itemId ? index : -1;
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

bool Inventory::changeMoney(int amount)
{
    if (amount == 0)
    {
        return true;
    }

    int money = mPoss->money + amount;
    if (money < 0)
    {
        return false;
    }

    prepare();

    mPoss->money = money;
    msg.writeByte(255);
    msg.writeLong(money);
    return true;
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

int Inventory::move(int slot1, int slot2, int amount)
{
    if (amount == 0 || slot1 == slot2 || slot2 >= INVENTORY_SLOTS)
    {
        return amount;
    }

    int i1 = getIndex(slot1);
    if (i1 < 0)
    {
        return amount;
    }

    prepare();

    InventoryItem &it1 = mPoss->inventory[i1];
    int i2 = getIndex(slot2);

    if (i2 >= 0)
    {
        InventoryItem &it2 = mPoss->inventory[i2];
        if (it1.itemId == it2.itemId)
        {
            // Move between two stacks of the same kind.
            int maxPerSlot = ItemManager::getItem(it1.itemId)->getMaxPerSlot();
            int nb = std::min(std::min(amount, (int)it1.amount), maxPerSlot - it2.amount);
            if (nb == 0)
            {
                return amount;
            }

            it1.amount -= nb;
            it2.amount += nb;
            amount -= nb;

            msg.writeByte(slot2 + EQUIP_CLIENT_INVENTORY);
            msg.writeShort(it2.itemId);
            msg.writeByte(it2.amount);

            msg.writeByte(slot1 + EQUIP_CLIENT_INVENTORY);
            if (it1.amount == 0)
            {
                msg.writeShort(0);
                freeIndex(i1);
            }
            else
            {
                msg.writeShort(it1.itemId);
                msg.writeByte(it1.amount);
            }
            return amount;
        }

        // Swap between two different stacks.
        if (it1.amount != amount)
        {
            return amount;
        }

        std::swap(it1, it2);

        msg.writeByte(slot1 + EQUIP_CLIENT_INVENTORY);
        msg.writeShort(it1.itemId);
        msg.writeByte(it1.amount);
        msg.writeByte(slot2 + EQUIP_CLIENT_INVENTORY);
        msg.writeShort(it2.itemId);
        msg.writeByte(it2.amount);
        return 0;
    }

    // Move some items to an empty slot.
    int id = it1.itemId;
    int nb = std::min((int)it1.amount, amount);
    it1.amount -= nb;
    amount -= nb;

    msg.writeByte(slot1 + EQUIP_CLIENT_INVENTORY);
    if (it1.amount == 0)
    {
        msg.writeShort(0);
        freeIndex(i1);
    }
    else
    {
        msg.writeShort(id);
        msg.writeByte(it1.amount);
    }

    // Fill second slot.
    msg.writeByte(slot2 + EQUIP_CLIENT_INVENTORY);
    msg.writeShort(id);
    msg.writeByte(nb);

    for (std::vector< InventoryItem >::iterator i = mPoss->inventory.begin(),
         i_end = mPoss->inventory.end(); i != i_end; ++i)
    {
        if (i->itemId)
        {
            --slot2;
            continue;
        }

        if (slot2 >= i->amount)
        {
            slot2 -= i->amount;
            continue;
        }

        assert(slot2 >= 0 && i + 1 != i_end);

        if (i->amount == 1)
        {
            // One single empty slot in the range.
            i->itemId = id;
            i->amount = nb;
            return amount;
        }

        InventoryItem it = { id, nb };
        --i->amount;

        if (slot2 == 0)
        {
            // First slot in an empty range.
            mPoss->inventory.insert(i, it);
            return amount;
        }

        if (slot2 == i->amount)
        {
            // Last slot in an empty range.
            mPoss->inventory.insert(i + 1, it);
            return amount;
        }

        InventoryItem it3 = { 0, slot2 };
        i->amount -= slot2;
        i = mPoss->inventory.insert(i, it);
        mPoss->inventory.insert(i, it3);
        return amount;
    }

    // The second slot does not yet exist.
    assert(slot2 >= 0);
    if (slot2 != 0)
    {
        InventoryItem it = { 0, slot2 };
        mPoss->inventory.insert(mPoss->inventory.end(), it);
    }
    InventoryItem it = { id, nb };
    mPoss->inventory.insert(mPoss->inventory.end(), it);
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

void Inventory::changeEquipment(int slot, int itemId)
{
    // FIXME: Changes are applied now, so it does not work in delayed mode.
    assert(!mDelayed);

    int oldId = mPoss->equipment[slot];
    if (oldId == itemId)
    {
        return;
    }

    if (oldId)
    {
        ItemManager::getItem(oldId)->getModifiers().cancelAttributes(mClient);
    }

    if (itemId)
    {
        ItemManager::getItem(itemId)->getModifiers().applyAttributes(mClient);
    }

    msg.writeByte(slot);
    msg.writeShort(itemId);
    mPoss->equipment[slot] = itemId;
    mChangedLook = true;

    //mark evade as modified because it depends on equipment weight
    mClient->modifiedAttribute(BASE_ATTR_EVADE);
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
            changeEquipment(EQUIP_FIGHT1_SLOT, itemId);
            changeEquipment(EQUIP_FIGHT2_SLOT, 0);
            return;
        }

        case ITEM_EQUIPMENT_AMMO:
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
    changeEquipment(firstSlot, itemId);
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
        changeEquipment(slot, 0);
    }
}
