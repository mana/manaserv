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

#ifndef _TMWSERV_ITEM
#define _TMWSERV_ITEM

#include "game-server/player.hpp"

/**
 * Enumeration of available Item types.
 */
enum
{
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
enum
{
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
enum
{
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
    unsigned char element; /**< Item Element */
    unsigned char beingStateEffect; /**< Being State (dis)alteration */
    unsigned short lifetime; /**< Modifiers lifetime in seconds. */

    // Caracteristics Modifiers
    short rawStats[NB_RSTAT]; /**< Raw Stats modifiers */
    short computedStats[NB_CSTAT]; /**< Computed Stats modifiers */

    short hp; /**< HP modifier */
    short mp; /**< MP Modifier */

    // Weapon
    unsigned short range; /**< Weapon Item Range */
    unsigned char weaponType; /**< Weapon Type enum */
};


/**
 * Class for simple reference to item information.
 */
class ItemClass
{
    public:
        ItemClass(int id, int type)
          : mDatabaseID(id), mType(type)
        {}

        /**
         * The function called to use an item applying
         * only the modifiers (for simple items...)
         */
        bool use(Being *itemUser);

        /**
         * Gets item type.
         */
        int getType() const
        { return mType; }

        /**
         * Gets item weight.
         */
        int getWeight() const
        { return mWeight; }

        /**
         * Sets item weight.
         */
        void setWeight(int weight)
        { mWeight = weight; }

        /**
         * Gets unit cost of these items.
         */
        int getCost() const
        { return mCost; }

        /**
         * Sets unit cost of these items.
         */
        void setCost(int cost)
        { mCost = cost; }

        /**
         * Gets max item per slot.
         */
        int getMaxPerSlot() const
        { return mMaxPerSlot; }

        /**
         * Sets max item per slot.
         */
        void setMaxPerSlot(int perSlot)
        { mMaxPerSlot = perSlot; }

        /**
         * Gets item modifiers.
         */
        Modifiers const &getModifiers() const
        { return mModifiers; }

        /**
         * Sets item modifiers.
         */
        void setModifiers(Modifiers const &modifiers)
        { mModifiers = modifiers; }

        /**
         * Sets associated script name.
         */
        void setScriptName(std::string const &name)
        { mScriptName = name; }

        /**
         * Gets database ID.
         */
        int getDatabaseID()
        { return mDatabaseID; }

    private:

        /**
         * Runs the associated script when using the item, if any.
         */
        bool runScript(Being *itemUser);

        // Item reference information
        unsigned short mDatabaseID;
        unsigned char mType;     /**< Type: usable, equipment. */
        unsigned short mWeight;  /**< Weight of the item. */
        unsigned short mCost;    /**< Unit cost the item. */
        unsigned short mMaxPerSlot; /**< Max item amount per slot in inventory. */
        std::string mScriptName; /**< Item script. */
        Modifiers mModifiers; /**< Item modifiers. */
};

class Item: public Object
{
    public:
        Item(ItemClass *type, int amount)
          : Object(OBJECT_ITEM), mType(type), mAmount(amount)
        {}

        ItemClass *getItemClass() const
        { return mType; }

        int getAmount() const
        { return mAmount; }

        virtual void update() {}

    private:
        ItemClass *mType;
        unsigned char mAmount;
};

#endif
