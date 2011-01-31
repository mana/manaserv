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

#include <algorithm>
#include <cassert>

#include "game-server/trade.h"

#include "game-server/character.h"
#include "game-server/gamehandler.h"
#include "game-server/inventory.h"
#include "net/messageout.h"

/*
 * States :
 * TRADE_INIT : A player has ask to make a trade, waiting for accept
 * TRADE_RUN : Both player are now trading and can add objects and GP (1)
 * TRADE_CONFIRM_WAIT : One player has confirm, waiting for the other one
 * TRADE_CONFIRMED : Both player has confirmed and agree button is unlock (2)
 * TRADE_AGREE_WAIT : One player has agreed, waiting for the other one
 */

Trade::Trade(Character *c1, Character *c2):
    mChar1(c1), mChar2(c2), mMoney1(0), mMoney2(0), mState(TRADE_INIT), mCurrencyId(ATTR_GP)
{
    MessageOut msg(GPMSG_TRADE_REQUEST);
    msg.writeInt16(c1->getPublicID());
    c2->getClient()->send(msg);
    c1->setTrading(this);
    c2->setTrading(this);
}

Trade::~Trade()
{
    mChar1->setTrading(NULL);
    mChar2->setTrading(NULL);
}

void Trade::cancel()
{
    MessageOut msg(GPMSG_TRADE_CANCEL);
    mChar1->getClient()->send(msg);
    mChar2->getClient()->send(msg);
    delete this;
}

bool Trade::request(Character *c, int id)
{
    //The trade isn't confirmed, the player which is request is the same.
    if (mState != TRADE_INIT || c != mChar2 || mChar1->getPublicID() != id)
    {
        /* This is not an ack for the current transaction. So assume
           a new one is about to start and cancel the current one. */
        cancel();
        return false;
    }

    //Second player confirmed.

    //Starts trading.
    mState = TRADE_RUN;

    //Telling both player that the trade has started
    MessageOut msg(GPMSG_TRADE_START);
    mChar1->getClient()->send(msg);
    mChar2->getClient()->send(msg);
    return true;
}

bool Trade::perform(TradedItems items, Inventory &inv1, Inventory &inv2)
{
    for (TradedItems::const_iterator i = items.begin(),
         i_end = items.end(); i != i_end; ++i)
    {
        if (i->id != inv1.getItem(i->slot) ||
            inv1.removeFromSlot(i->slot, i->amount) != 0 ||
            inv2.insert(i->id, i->amount) != 0)
        {
            return false;
        }
    }
    return true;
}

void Trade::agree(Character *c)
{
    // No player agreed
    if (mState == TRADE_CONFIRMED)
    {
        // One player agreed, if it's the player 2, make it player 1
        if (c == mChar2)
        {
            std::swap(mChar1, mChar2);
            std::swap(mItems1, mItems2);
            std::swap(mMoney1, mMoney2);
        }
        // First player agrees.
        mState = TRADE_CONFIRM_WAIT;

        // Send the other player that the first player has confirmed
        MessageOut msg(GPMSG_TRADE_AGREED);
        mChar2->getClient()->send(msg);
        return;
    }

    if (mState == TRADE_AGREE_WAIT && c == mChar1)
    {
        // We don't care about the first player, he already agreed
        return;
    }

    // The second player has agreed

    // Check if both player has the objects in their inventories
    // and enouth money, then swap them.
    Inventory v1(mChar1, true), v2(mChar2, true);
    if (mChar1->getAttribute(mCurrencyId) >= mMoney1 - mMoney2 &&
        mChar2->getAttribute(mCurrencyId) >= mMoney2 - mMoney1 &&
        perform(mItems1, v1, v2) &&
        perform(mItems2, v2, v1))
    {
        mChar1->setAttribute(mCurrencyId, mChar1->getAttribute(mCurrencyId)
                             - mMoney1 + mMoney2);
        mChar2->setAttribute(mCurrencyId, mChar2->getAttribute(mCurrencyId)
                             - mMoney2 + mMoney1);
    }
    else
    {
        v1.cancel();
        v2.cancel();
        cancel();
        return;
    }

    MessageOut msg(GPMSG_TRADE_COMPLETE);
    mChar1->getClient()->send(msg);
    mChar2->getClient()->send(msg);
    delete this;
}

void Trade::confirm(Character *c)
{
    if (mState == TRADE_CONFIRMED || mState == TRADE_AGREE_WAIT)
        return;

    if (mState == TRADE_RUN) //No player has confirmed
    {
        //One player confirms, if it's the player 2, make it player 1
        if (c == mChar2)
        {
            std::swap(mChar1, mChar2);
            std::swap(mItems1, mItems2);
            std::swap(mMoney1, mMoney2);
        }
        assert(c == mChar1);
        // First player agrees.
        mState = TRADE_CONFIRM_WAIT;

        //Send the other player that the first player has confirmed
        MessageOut msg(GPMSG_TRADE_CONFIRM);
        mChar2->getClient()->send(msg);
        return;
    }

    if (mState != TRADE_CONFIRM_WAIT || c != mChar2)
    {
        // First player has already agreed. We only care about the second one.
        return;
    }

    mState = TRADE_CONFIRMED;
    MessageOut msg(GPMSG_TRADE_BOTH_CONFIRM);
    mChar1->getClient()->send(msg);
    mChar2->getClient()->send(msg);
}

void Trade::setMoney(Character *c, int amount)
{
    //If the player has already confirmed, exit.
    if ((mState != TRADE_RUN && (mState != TRADE_CONFIRM_WAIT || c != mChar1))
            || amount < 0) return;

    /* Checking now if there is enough money is useless as it can change
       later on. At worst, the transaction will be canceled at the end if
       the client lied. */

    MessageOut msg(GPMSG_TRADE_SET_MONEY);
    msg.writeInt32(amount);

    if (c == mChar1)
    {
        mMoney1 = amount;
        mChar2->getClient()->send(msg);
    }
    else
    {
        assert(c == mChar2);
        mMoney2 = amount;
        mChar1->getClient()->send(msg);
    }

    // Go back to normal run.
    mState = TRADE_RUN;
}

void Trade::addItem(Character *c, int slot, int amount)
{
    //If the player has already confirmed, exit.
    if ((mState != TRADE_RUN && (mState != TRADE_CONFIRM_WAIT || c != mChar1))
            || amount < 0) return;

    Character *other;
    TradedItems *items;
    if (c == mChar1)
    {
        other = mChar2;
        items = &mItems1;
    }
    else
    {
        assert(c == mChar2);
        other = mChar1;
        items = &mItems2;
    }

    // Arbitrary limit to prevent a client from DOSing the server.
    if (items->size() >= 50) return;

    Inventory inv(c);
    int id = inv.getItem(slot);
    if (id == 0) return;

    /* Checking now if there is enough items is useless as it can change
       later on. At worst, the transaction will be canceled at the end if
       the client lied. */

    TradedItem ti = { id, slot, amount };
    items->push_back(ti);

    MessageOut msg(GPMSG_TRADE_ADD_ITEM);
    msg.writeInt16(id);
    msg.writeInt8(amount);
    other->getClient()->send(msg);
}
