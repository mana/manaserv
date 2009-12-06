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

#ifndef GAMESERVER_BUYSELL_HPP
#define GAMESERVER_BUYSELL_HPP

#include <vector>

class Character;
class Actor;

class BuySell
{
    public:

        /**
         * Sets up a trade between a character and an NPC.
         */
        BuySell(Character *, bool sell);

        /**
         * Cancels the trade.
         */
        void cancel();

        /**
         * Registers an item and indicates how many the NPC is ready to trade
         * and how much it will cost.
         */
        void registerItem(int id, int amount, int cost);

        /**
         * Sends the item list to player.
         */
        void start(Actor *actor);

        /**
         * Performs the trade.
         */
        void perform(int id, int amount);

    private:

        ~BuySell();

        struct TradedItem
        {
            unsigned short itemId, amount, cost;
        };

        typedef std::vector< TradedItem > TradedItems;

        Character *mChar;   /**< Character involved. */
        TradedItems mItems; /**< Traded items. */
        bool mSell;         /**< Are items sold? */
};

#endif
