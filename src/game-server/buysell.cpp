/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
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

#include "game-server/buysell.h"

#include "game-server/character.h"
#include "game-server/gamehandler.h"
#include "game-server/inventory.h"
#include "game-server/itemmanager.h"
#include "game-server/item.h"
#include "net/messageout.h"
#include "common/defines.h"

#include <algorithm>

BuySell::BuySell(Character *c, bool sell):
    mCurrencyId(ATTR_GP), mChar(c), mSell(sell)
{
    c->setBuySell(this);
}

BuySell::~BuySell()
{
    mChar->setBuySell(NULL);
}

void BuySell::cancel()
{
    delete this;
}

bool BuySell::registerItem(int id, int amount, int cost)
{
    if (mSell)
    {
        int nb = Inventory(mChar).count(id);
        if (nb == 0)
            return false;
        if (!amount || nb < amount)
            amount = nb;
    }

    TradedItem it = { id, amount, cost };
    mItems.push_back(it);
    return true;
}


int BuySell::registerPlayerItems()
{
    if (!mSell)
        return 0;

    int nbItemsToSell = 0;

    // We parse the player inventory and add all item
    // in a sell list.
    const InventoryData &inventoryData = mChar->getPossessions().getInventory();
    for (InventoryData::const_iterator it = inventoryData.begin(),
        it_end = inventoryData.end(); it != it_end; ++it)
    {
        unsigned int nb = it->second.amount;
        if (!nb)
            continue;

        int id = it->second.itemId;
        int cost = -1;
        if (itemManager->getItem(id))
        {
            cost = itemManager->getItem(id)->getCost();
        }
        else
        {
            LOG_WARN("registerPlayersItems(): The character Id: "
                << mChar->getPublicID() << " has unknown items (Id: " << id
                << "). They have been ignored.");
            continue;
        }

        if (cost < 1)
            continue;

        // We check if the item Id has been already
        // added. If so, we cumulate the amounts.
        bool itemAlreadyAdded = false;
        for (TradedItems::iterator i = mItems.begin(),
            i_end = mItems.end(); i != i_end; ++i)
        {
            if (i->itemId == id)
            {
                itemAlreadyAdded = true;
                i->amount += nb;
                break;
            }
        }

        if (!itemAlreadyAdded)
        {
            TradedItem itTrade = { id, nb, cost };
            mItems.push_back(itTrade);
            nbItemsToSell++;
        }
    }
    return nbItemsToSell;
}

bool BuySell::start(Actor *actor)
{
    if (mItems.empty())
    {
        cancel();
        return false;
    }

    MessageOut msg(mSell ? GPMSG_NPC_SELL : GPMSG_NPC_BUY);
    msg.writeInt16(actor->getPublicID());
    for (TradedItems::const_iterator i = mItems.begin(),
         i_end = mItems.end(); i != i_end; ++i)
    {
        msg.writeInt16(i->itemId);
        msg.writeInt16(i->amount);
        msg.writeInt16(i->cost);
    }
    mChar->getClient()->send(msg);
    return true;
}

void BuySell::perform(int id, int amount)
{
    Inventory inv(mChar);
    for (TradedItems::iterator i = mItems.begin(),
         i_end = mItems.end(); i != i_end; ++i)
    {
        if (i->itemId != id) continue;
        if (i->amount && i->amount <= amount) amount = i->amount;
        if (mSell)
        {
            amount -= inv.remove(id, amount);
            mChar->setAttribute(mCurrencyId,
                                mChar->getAttribute(mCurrencyId) +
                                amount * i->cost);
        }
        else
        {
            amount = std::min(amount, ((int) mChar->getAttribute(mCurrencyId)) / i->cost);
            amount -= inv.insert(id, amount);
            mChar->setAttribute(mCurrencyId,
                                mChar->getAttribute(mCurrencyId) -
                                amount * i->cost);
        }
        if (i->amount)
        {
            i->amount -= amount;
            if (!i->amount)
            {
                mItems.erase(i);
            }
        }
        return;
    }
}
