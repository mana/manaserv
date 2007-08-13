/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
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

#include "game-server/buysell.hpp"

#include "defines.h"
#include "game-server/character.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/inventory.hpp"
#include "net/messageout.hpp"

BuySell::BuySell(Character *c, bool sell):
    mChar(c), mSell(sell)
{
    c->setBuySell(this);
}

BuySell::~BuySell()
{
    mChar->cancelTransaction();
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
        if (nb == 0) return;
        if (!amount || nb < amount) amount = nb;
    }

    TradedItem it = { id, amount, cost };
    mItems.push_back(it);
}

void BuySell::start(MovingObject *obj)
{
    if (mItems.empty())
    {
        cancel();
        return;
    }

    MessageOut msg(mSell ? GPMSG_NPC_SELL : GPMSG_NPC_BUY);
    msg.writeShort(obj->getPublicID());
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
    int money = mChar->getMoney();
    for (TradedItems::iterator i = mItems.begin(),
         i_end = mItems.end(); i != i_end; ++i)
    {
        if (i->itemId != id) continue;
        if (i->amount && i->amount <= amount) amount = i->amount;
        if (mSell)
        {
            amount -= inv.remove(id, amount);
            money += amount * i->cost;
        }
        else
        {
            amount = std::min(amount, money / i->cost);
            amount -= inv.insert(id, amount);
            money -= amount * i->cost;
        }
        if (i->amount)
        {
            i->amount -= amount;
            if (!i->amount)
            {
                mItems.erase(i);
            }
        }
        mChar->setMoney(money);
        return;
    }
}
