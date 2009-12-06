/*
 *  The Mana Server
 *  Copyright (C) 2007  The Mana World Development Team
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
#include "net/messageout.hpp"

#include <algorithm>

BuySell::BuySell(Character *c, bool sell):
    mChar(c), mSell(sell)
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

void BuySell::registerItem(int id, int amount, int cost)
{
    if (mSell)
    {
        int nb = Inventory(mChar).count(id);
        if (nb == 0)
            return;
        if (!amount || nb < amount)
            amount = nb;
    }

    TradedItem it = { id, amount, cost };
    mItems.push_back(it);
}

void BuySell::start(Actor *actor)
{
    if (mItems.empty())
    {
        cancel();
        return;
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
            inv.changeMoney(amount * i->cost);
        }
        else
        {
            amount = std::min(amount, mChar->getPossessions().money / i->cost);
            amount -= inv.insert(id, amount);
            inv.changeMoney(-amount * i->cost);
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
