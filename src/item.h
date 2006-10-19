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
typedef enum {
    ITEM_USABLE = 1,
    ITEM_EQUIPMENT_ONE_HAND_WEAPON,
    ITEM_EQUIPMENT_TWO_HANDS_WEAPON,
    ITEM_EQUIPMENT_ONE_HAND_RANGED_WEAPON,
    ITEM_EQUIPMENT_TWO_HANDS_RANGED_WEAPON,
    ITEM_EQUIPMENT_BREST,
    ITEM_EQUIPMENT_ARMS,
    ITEM_EQUIPMENT_HEAD,
    ITEM_EQUIPMENT_LEGS,
    ITEM_EQUIPMENT_SHIELD,
    ITEM_EQUIPMENT_RING,
    ITEM_EQUIPMENT_NECKLACE
} ItemType;

/**
 * States attribute effects to beings, and actors.
 * States can be multiple for the same being.
 */
struct BeingStateEffects {
    bool STATE_NORMAL;
    bool STATE_POISONED;
    bool STATE_STONED;
    bool STATE_STUNNED;
    bool STATE_SLOWED;
    bool STATE_TIRED;
    bool STATE_MAD;
    bool STATE_BERSERK;
    bool STATE_HASTED;
    bool STATE_FLOATING;

    bool STATE_NOT_POISONED;
    bool STATE_NOT_STONED;
    bool STATE_NOT_STUNNED;
    bool STATE_NOT_SLOWED;
    bool STATE_NOT_TIRED;
    bool STATE_NOT_MAD;
    bool STATE_NOT_BERSERK;
    bool STATE_NOT_HASTED;
    bool STATE_NOT_FLOATING;
};


/**
 * statistics modifiers.
 * once for usables.
 * Permanent for equipment.
 */
struct Modifiers
{
    // General
    Element element; /** Item Element */
    BeingStateEffects beingStateEffects; /** Being State (dis)alteration */
    unsigned short lifetime; /** Modifiers lifetime in seconds. */

    // Caracteristics Modifiers
    short rawStats[NB_RSTAT];
    short computedStats[NB_CSTAT];

    int hpMod; /**< HP modifier */
    int mpMod; /**< MP Modifier */

    // Equipment
    unsigned short range; /** Weapon Item Range */
    /**< More to come */
};


/**
 * Class for simple reference to item information.
 * See WorldItem to get full featured Item Objects.
 */
class Item
{
    public:

        Item(Modifiers modifiers,
            unsigned short itemType = 0,
            unsigned int weight = 0,
            unsigned int value = 0,
            std::string scriptName = ""):
            mModifiers(modifiers),
            mItemType(itemType),
            mWeight(weight),
            mValue(value),
            mScriptName(scriptName) {}

        virtual ~Item() throw() { }

        /**
         * The function called to use an item applying
         * only the modifiers (for simple items...)
         */
        bool use(BeingPtr itemUser);

        /**
         * Return item Type
         */
        unsigned short getItemType() const { return mItemType; };

        /**
         * Return Weight of item
         */
        unsigned int getWeight() const { return mWeight; };

        /**
         * Return gold value of item
         */
        unsigned int getGoldValue() const { return mValue; };

        /**
         * Return item's modifiers
         */
        Modifiers
        getItemModifiers() const { return mModifiers; };

    private:

        /**
         * Runs the associated script when using the item, if any.
         */
        bool runScript(BeingPtr itemUser);

        // Item reference information
        unsigned short mItemType; /**< ItemType: Usable, equipment */
        unsigned int mWeight; /**< Weight of the item */
        unsigned int mValue; /**< Gold value of the item */
        std::string mScriptName; /**< item's script. None if =="" */

        Modifiers mModifiers; /**< Item's Modifiers */
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
