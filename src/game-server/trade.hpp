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
 */

#ifndef _TMWSERV_GAMESERVER_TRADE_HPP_
#define _TMWSERV_GAMESERVER_TRADE_HPP_

#include <vector>

class Character;
class Inventory;

class Trade
{
    public:

        /**
         * Sets up a trade between two characters.
         * Asks for an acknowledgment from the second one.
         */
        Trade(Character *, Character *);

        /**
         * Cancels a trade by a given character (optional).
         * Warns the other character the trade is cancelled.
         * Takes care of cleaning afterwards.
         */
        void cancel(Character *);

        /**
         * Requests a trade to start with given public ID.
         * Continues the current trade if the ID is correct, cancels it
         * otherwise.
         * @return true if the current trade keeps going.
         */
        bool request(Character *, int);

        /**
         * Agrees to complete the trade.
         */
        void accept(Character *);

        /**
         * Adds some items to the trade.
         */
        void addItem(Character *, int slot, int amount);

        /**
         * Adds some money to the trade.
         */
        void setMoney(Character *, int amount);

    private:

        ~Trade();

        struct TradedItem
        {
            unsigned short id;
            unsigned char slot, amount;
        };

        typedef std::vector< TradedItem > TradedItems;

        enum TradeState
        {
            TRADE_INIT = 0, /**< Waiting for an ack from player 2. */
            TRADE_RUN,      /**< Currently trading. */
            TRADE_EXIT      /**< Waiting for an ack from player 2. */
        };

        static bool perform(TradedItems items, Inventory &inv1, Inventory &inv2);

        Character *mChar1, *mChar2;   /**< Characters involved. */
        TradedItems mItems1, mItems2; /**< Traded items. */
        int mMoney1, mMoney2;         /**< Traded money. */
        TradeState mState;            /**< State of transaction. */
};

#endif
