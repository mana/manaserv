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

#ifndef ITEM_H
#define ITEM_H

#include "being.h"

/**
 * Enumeration of available Item types.
 */
typedef enum ItemType {
    ITEM_UNUSABLE = 0,
    ITEM_USABLE,                            // 1
    ITEM_EQUIPMENT_ONE_HAND_WEAPON,         // 2
    ITEM_EQUIPMENT_TWO_HANDS_WEAPON,        // 3
    ITEM_EQUIPMENT_BREST,                   // 4
    ITEM_EQUIPMENT_ARMS,                    // 5
    ITEM_EQUIPMENT_HEAD,                    // 6
    ITEM_EQUIPMENT_LEGS,                    // 7
    ITEM_EQUIPMENT_SHIELD,                  // 8
    ITEM_EQUIPMENT_RING,                    // 9
    ITEM_EQUIPMENT_NECKLACE,                // 10
    ITEM_EQUIPMENT_FEET,                    // 11
    ITEM_EQUIPMENT_PROJECTILE               // 12
};

/**
 * Enumeration of available weapon's types.
 */
typedef enum WeaponType {
    WPNTYPE_NONE = 0,
    WPNTYPE_KNIFE,          // 1
    WPNTYPE_SWORD,          // 2
    WPNTYPE_SPEAR,          // 3
    WPNTYPE_JAVELIN,        // 4
    WPNTYPE_ROD,            // 5
    WPNTYPE_STAFF,          // 6
    WPNTYPE_WIPE,           // 7
    WPNTYPE_PROJECTILE,     // 8
    WPNTYPE_BOOMERANG,      // 9
    WPNTYPE_BOW,            // 10
    WPNTYPE_SICKLE,         // 11
    WPNTYPE_CROSSBOW,       // 12
    WPNTYPE_STICK,          // 13
    WPNTYPE_HAMMER,         // 14
    WPNTYPE_AXE,            // 15
    WPNTYPE_HAND_PROECTILE  // 16
};

/**
 * States attribute effects to beings, and actors.
 * States can be multiple for the same being.
 */
typedef enum BeingStateEffect {
    STATE_NORMAL = 0,
    STATE_POISONED,
    STATE_STONED,
    STATE_STUNNED,
    STATE_SLOWED,
    STATE_TIRED,
    STATE_MAD,
    STATE_BERSERK,
    STATE_HASTED,
    STATE_FLOATING,

    STATE_NOT_POISONED,
    STATE_NOT_STONED,
    STATE_NOT_STUNNED,
    STATE_NOT_SLOWED,
    STATE_NOT_TIRED,
    STATE_NOT_MAD,
    STATE_NOT_BERSERK,
    STATE_NOT_HASTED,
    STATE_NOT_FLOATING
};

/**
 * statistics modifiers.
 * once for usables.
 * Permanent for equipment.
 */
struct Modifiers
{
    // General
    Element element; /**< Item Element */
    BeingStateEffect beingStateEffect; /**< Being State (dis)alteration */
    unsigned short lifetime; /**< Modifiers lifetime in seconds. */

    // Caracteristics Modifiers
    short rawStats[NB_RSTAT]; /**< Raw Stats modifiers */
    short computedStats[NB_CSTAT]; /**< Computed Stats modifiers */

    int hp; /**< HP modifier */
    int mp; /**< MP Modifier */

    // Weapon
    unsigned short range; /**< Weapon Item Range */
    WeaponType weaponType; /**< Weapon Type enum */
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
            std::string scriptName = "",
            unsigned short maxPerSlot = 0):
            mItemType(itemType),
            mWeight(weight),
            mValue(value),
            mScriptName(scriptName),
            mMaxPerSlot(maxPerSlot),
            mModifiers(modifiers) {}

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
         * Return max item per slot
         */
        unsigned short getMaxPerSlot() const { return mMaxPerSlot; };

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
        unsigned short mMaxPerSlot; /**< Max item amount per slot in inventory */

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
