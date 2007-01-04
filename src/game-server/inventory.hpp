/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
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

#ifndef INVENTORY_H
#define INVENTORY_H

#include "playerdata.hpp"

enum
{
// Equipment rules:
// 1 Brest equipment
    EQUIP_BREST_SLOT = 0,
// 1 arms equipment
    EQUIP_ARMS_SLOT = 1,
// 1 head equipment
    EQUIP_HEAD_SLOT = 2,
// 1 legs equipment
    EQUIP_LEGS_SLOT = 3,
// 1 feet equipment
    EQUIP_FEET_SLOT = 4,
// 2 rings
    EQUIP_RING1_SLOT = 5,
    EQUIP_RING2_SLOT = 6,
// 1 necklace
    EQUIP_NECKLACE_SLOT = 7,
// Fight:
//   2 one-handed weapons
//   or 1 two-handed weapon
//   or 1 one-handed weapon + 1 shield.
    EQUIP_FIGHT1_SLOT = 8,
    EQUIP_FIGHT2_SLOT = 9,
// Projectile:
//   this item does not amount to one, it only indicates the chosen projectile.
    EQUIP_PROJECTILE_SLOT = 10,

    EQUIP_CLIENT_INVENTORY = 32
};

class MessageOut;

/**
 * Class used to handle Player possessions and prepare outgoing messages.
 */
class Inventory
{
        Possessions &poss;
        MessageOut &msg;
    public:
        Inventory(PlayerData *p, MessageOut &m)
          : poss(p->getPossessions()), msg(m)
        {}

        /**
         * Equips item from given inventory slot.
         */
        bool equip(int slot);

        /**
         * Gets the ID of projectiles. Removes one of these projectiles from
         * inventory.
         */
        int fireProjectile();

        /**
         * Inserts some items into the inventory.
         * @return number of items not inserted (to be dropped on floor?).
         */
        int insert(int itemId, int amount);

        /**
         * Removes some items from inventory.
         * @return number of items not removed.
         */
        int remove(int itemId, int amount);

        int countItem(int itemId);
    private:
        /**
         * Fills some slots with items.
         * @return number of items not inserted.
         */
        int fillFreeSlot(int itemId, int amount, int MaxPerSlot);
        void freeIndex(int index);
        int getItem(int slot);
        int getIndex(int slot);
        int getSlot(int index);
};


#endif
