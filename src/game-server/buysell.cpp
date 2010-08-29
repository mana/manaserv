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

#include "game-server/buysell.hpp"

#include "game-server/character.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/inventory.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/item.hpp"
#include "net/messageout.hpp"

#include "defines.h"

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
    return 0; // FIXME: STUB
    /*
     * Replaced with a no-op stub after the equipment slots become softcoded.
     * I think this function is meant to fill the sell dialog with player
     *     items, but it's iterating through the inventory.
     * The no-op here is to stop compilation errors while I work on other
     *     areas. FIXME
     */
    /*
    int nbItemsToSell = 0;
    if (mSell)
    {
        for (int i = 0; i < EQUIPMENT_SLOTS; ++i)
        {
            int id = Inventory(mChar).getItem(i);
            int nb = Inventory(mChar).count(id);
            if (nb > 0)
            {
                int cost = -1;
                if (ItemManager::getItem(id))
                    cost = ItemManager::getItem(id)->getCost();
                if (cost > 0)
                {
                    TradedItem it = { id, nb, cost };
                    mItems.push_back(it);
                    nbItemsToSell++;
                }
            }
        }
    }
    return nbItemsToSell;
    */
}

bool BuySell::start(Actor *actor)
{
    if (mItems.empty())
    {
        cancel();
        return false;
    }

    MessageOut msg(mSell ? GPMSG_NPC_SELL : GPMSG_NPC_BUY);
    msg.writeShort(actor->getPublicID());
    for (TradedItems::const_iterator i = mItems.begin(),
         i_end = mItems.end(); i != i_end; ++i)
    {
        msg.writeShort(i->itemId);
        msg.writeShort(i->amount);
        msg.writeShort(i->cost);
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
