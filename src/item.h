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
 *  $Id: items.h 2636 2006-09-02 12:03:22Z gmelquio $
 */

#ifndef ITEM_H
#define ITEM_H

#include "being.h"

/**
 * Enumeration of available Item types.
 */
enum {
    ITEM_USABLE = 0,
    ITEM_EQUIPMENT_WEAPON,
    ITEM_EQUIPMENT_BREST,
    ITEM_EQUIPMENT_ARMS,
    ITEM_EQUIPMENT_HEAD,
    ITEM_EQUIPMENT_LEGS,
    ITEM_EQUIPMENT_SHIELD,
    ITEM_EQUIPMENT_RING,
    ITEM_EQUIPMENT_NECKLACE
};

/**
 * statistics modifiers.
 * once for usables.
 * Permanent for equipment.
 */
struct StatisticsModifiers
{
    short rawStatsMod[NB_RSTAT]; /**< Raw Stats modifiers */
    short compStatsMod[NB_CSTAT]; /**< Computed Stats modifiers */
    int hpMod; /**< HP modifier */
    int mpMod; /**< MP Modifier */
    /**< More to come */
};


/**
 * Class for simple reference to item information.
 * See WorldItem to get full featured Item Objects.
 */
class Item
{
    public:

        Item(StatisticsModifiers statsModifiers,
            unsigned short mItemType = 0,
            unsigned int weight = 0,
            unsigned short slot = 0,
            unsigned int value = 0):
            mWeight(weight),
            mSlot(slot),
            mValue(value),
            mStatsModifiers(statsModifiers) {}

        virtual ~Item() throw() { }

        /**
         * The function called to use an item applying
         * only the modifiers (for simple items...)
         */
        void use(BeingPtr itemUser);

        /**
         * The function called to use an item
         * using a script (for complex actions)
         */
        void useWithScript(const std::string scriptFile);

        /**
         * Return item Type
         */
        unsigned short getItemType() const { return mItemType; }

        /**
         * Return Weight of item
         */
        unsigned int getWeight() const { return mWeight; }

        /**
         * Return usual slot of item
         */
        unsigned short getSlot() const { return mSlot; }

        /**
         * Return gold value of item
         */
        unsigned int getGoldValue() const { return mValue; }

        /**
         * Return item's modifiers
         */
        StatisticsModifiers
        getItemStatsModifiers() const { return mStatsModifiers; }

    private:
        //Item type
        unsigned short mItemType; /**< ItemType: Usable, equipment */
        unsigned int mWeight; /**< Weight of the item */
        unsigned short mSlot; /**< Current slot of the item */
        unsigned int mValue; /**< Gold value of the item */
        StatisticsModifiers mStatsModifiers; /**< Stats Mod of the items */
};

/**
 * Type definition for a smart pointer to Item.
 */
typedef utils::CountedPtr<Item> ItemPtr;

/**
 * Type definition for a list of Items.
 */
typedef std::vector<ItemPtr> Items;

#endif
