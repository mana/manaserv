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

#ifndef _TMWSERV_GAMESERVER_BUYSELL_HPP_
#define _TMWSERV_GAMESERVER_BUYSELL_HPP_

#include <vector>

class Character;
class MovingObject;

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
        void start(MovingObject *obj);

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
