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

#ifndef INVENTORY_H
#define INVENTORY_H

#include "game-server/character.h"
#include "net/messageout.h"

class ItemClass;

/**
 * Class used to handle Character possessions and prepare outgoing messages.
 */
class Inventory
{
    public:

        /**
         * Creates a view on the possessions of a character.
         * @param delayed If the changes need to be cancelable.
         */
        Inventory(Character *, bool delayed = false);

        /**
         * Commits delayed changes if applicable.
         * Sends the update message to the client.
         */
        ~Inventory();

        /**
         * Commits changes.
         * Exclusive to delayed mode.
         * @param doRestart Whether to prepare the inventory for more changes
                   after this. If you are unsure, it is safe (though not
                   terribly efficient) to leave this as true.
         */
        void commit(bool doRestart = true);

        /**
         * Cancels changes.
         * Exclusive to delayed mode.
         */
        void cancel();

        /**
         * Sends complete inventory status to the client.
         */
        void sendFull() const;

        /**
         * Ensures the inventory is sane and apply equipment modifiers.
         * Should be run only once and the very first time.
         */
        void initialize();

        /**
         * Equips item from given inventory slot.
         * @param slot The slot in which the target item is in.
         * @param override Whether this item can unequip other items to equip
         *            itself. If true, items that are unequipped will be
         *            attempted to be reequipped, but with override disabled.
         * @returns whether the item could be equipped.
         */
        bool equip(int slot, bool override = true);

        /**
         * Unequips item from given equipment slot.
         * @param it Starting iterator. When the only parameter, also extracts
         *           slot number from it.
         *           Used so that when we already have an iterator to the first
         *           occurence from a previous operation we can start from
         *           there.
         * @returns Whether it was unequipped.
         */
        bool unequip(EquipData::iterator it);
        bool unequip(unsigned int slot, EquipData::iterator *itp = 0);

        /**
         * Inserts some items into the inventory.
         * @return number of items not inserted (to be dropped on floor?).
         */
        unsigned int insert(unsigned int itemId, unsigned int amount);

        /**
         * Removes some items from inventory.
         * @param force If set to true, also remove any equipment encountered
         * @return number of items not removed.
         */
        unsigned int remove(unsigned int itemId, unsigned int amount, bool force = false);

        /**
         * Moves some items from the first slot to the second one.
         * @returns number of items not moved.
         */
        unsigned int move(unsigned int slot1, unsigned int slot2, unsigned int amount);

        /**
         * Removes some items from inventory.
         * @return number of items not removed.
         */
        unsigned int removeFromSlot(unsigned int slot, unsigned int amount);

        /**
         * Counts number of items with given ID.
         */
        unsigned int count(unsigned int itemId) const;

        /**
         * Gets the ID of the items in a given slot.
         */
        unsigned int getItem(unsigned int slot) const;

    private:

        /**
         * Make sure that changes are being done on a copy, not directly.
         * No effect when not in delayed mode.
         */
        void prepare();

        /**
         * Starts a new notification message.
         */
        void restart();


        /**
         * Check the inventory is within the slot limit and capacity.
         * Forcibly delete items from the end if it is not.
         * @todo Drop items instead?
         */
        void checkInventorySize();

        /**
         * Helper function for equip() when computing changes to equipment
         * When newCount is 0, the item is being unequipped.
         */
        // inventory slot -> {equip slots}
        typedef std::multimap<unsigned int, unsigned short> IdSlotMap;
        void equip_sub(unsigned int newCount, IdSlotMap::const_iterator &it);

        /**
         * Changes equipment and adjusts character attributes.
         */
        void changeEquipment(unsigned int oldId, unsigned int itemId);
        void changeEquipment(ItemClass *oldI, ItemClass *newI);

        Possessions *mPoss; /**< Pointer to the modified possessions. */
        /**
         * Update message containing inventory changes.
         * Note that in sendFull(), this is reused to send all full changes
         * (for both inventory and equipment)
         */
        MessageOut mInvMsg;
        MessageOut mEqmMsg; /**< Update message containing equipment changes */
        Character *mCharacter; /**< Character to notify. */
        bool mDelayed;      /**< Delayed changes. */
};

#endif
