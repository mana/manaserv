/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */

#ifndef _TMWSERV_PLAYER_H_
#define _TMWSERV_PLAYER_H_

#include <string>
#include <vector>

#include "playerdata.hpp"
#include "game-server/being.hpp"
#include "game-server/inventory.hpp"

class GameClient;

class Player : public Being, public PlayerData
{
    public:

        Player(std::string const &name, int id = -1)
          : Being(OBJECT_PLAYER, 65535),
            PlayerData(name, id),
            mClient(NULL),
            mIsAttacking(false)
        {}

        /**
         * Updates the internal status.
         */
        void update();

        /**
         * Sets inventory.
         */
        void
        setInventory(const Inventory &inven);

        /**
         * Adds item with ID to inventory.
         *
         * @return Item add success/failure
         */
        bool
        addItem(unsigned int itemId, unsigned char amount = 1);

        /**
         * Removes item with ID from inventory.
         *
         * @return Item delete success/failure
         */
        bool
        removeItem(unsigned int itemId, unsigned char amount = 0);

        /**
         * Checks if character has an item.
         *
         * @return true if being has item, false otherwise
         */
        bool
        hasItem(unsigned int itemId);

        /**
         * Equips item with ID in equipment slot.
         *
         * @return Equip success/failure
         */
        bool
        equip(unsigned char slot);

        /**
         * Un-equips item.
         *
         * @return Un-equip success/failure
         */
        bool
        unequip(unsigned char slot);

        /**
         * Set attacking state
         **/
        void setAttacking(bool isAttacking)
        { mIsAttacking = isAttacking; }

        /**
         * Gets client computer.
         */
        GameClient *getClient() const
        { return mClient; }

        /**
         * Sets client computer.
         */
        void setClient(GameClient *c)
        { mClient = c; }

    private:
        Player(Player const &);
        Player &operator=(Player const &);

        GameClient *mClient;   /**< Client computer. */
        Inventory inventory;   /**< Player inventory and equipment. */
        bool mIsAttacking;     /**< Attacking state. */
};

#endif // _TMWSERV_PLAYER_H_
