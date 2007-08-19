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

#include <string>
#include <vector>

#include "game-server/object.hpp"

class Being;

/**
 * Enumeration of available Item types.
 */
enum
{
    ITEM_UNUSABLE = 0,
    ITEM_USABLE,                            // 1
    ITEM_EQUIPMENT_ONE_HAND_WEAPON,         // 2
    ITEM_EQUIPMENT_TWO_HANDS_WEAPON,        // 3
    ITEM_EQUIPMENT_TORSO,                   // 4
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
    WPNTYPE_WHIP,           // 7
    WPNTYPE_PROJECTILE,     // 8
    WPNTYPE_BOOMERANG,      // 9
    WPNTYPE_BOW,            // 10
    WPNTYPE_SICKLE,         // 11
    WPNTYPE_CROSSBOW,       // 12
    WPNTYPE_STICK,          // 13
    WPNTYPE_HAMMER,         // 14
    WPNTYPE_AXE,            // 15
    WPNTYPE_HAND_PROJECTILE  // 16
};

/**
 * State effects to beings, and actors.
 * States can be multiple for the same being.
 */
enum
{
    SET_STATE_NORMAL = 0,
    SET_STATE_POISONED,
    SET_STATE_STONED,
    SET_STATE_STUNNED,
    SET_STATE_SLOWED,
    SET_STATE_TIRED,
    SET_STATE_MAD,
    SET_STATE_BERSERK,
    SET_STATE_HASTED,
    SET_STATE_FLOATING,

    SET_STATE_NOT_POISONED,
    SET_STATE_NOT_STONED,
    SET_STATE_NOT_STUNNED,
    SET_STATE_NOT_SLOWED,
    SET_STATE_NOT_TIRED,
    SET_STATE_NOT_MAD,
    SET_STATE_NOT_BERSERK,
    SET_STATE_NOT_HASTED,
    SET_STATE_NOT_FLOATING
};

/**
 * Item modifier types.
 */
enum
{
    MOD_WEAPON_TYPE = 0,
    MOD_WEAPON_RANGE,
    MOD_WEAPON_DAMAGE,
    MOD_ELEMENT_TYPE,
    MOD_LIFETIME,
    MOD_ATTRIBUTE
};

/**
 * Characteristic of an item.
 */
struct ItemModifier
{
    unsigned char type;
    short value;
};

/**
 * Set of item characteristics.
 */
class ItemModifiers
{
    public:

        /**
         * Gets the value associated to a modifier type, or zero if none.
         */
        int getValue(int type) const;

        /**
         * Sets the value associated to a modifier type.
         */
        void setValue(int type, int amount);

        /**
         * Gets the value associated to a MOD_ATTRIBUTE class, or zero if none.
         */
        int getAttributeValue(int attr) const;

        /**
         * Sets the value associated to a MOD_ATTRIBUTE class.
         */
        void setAttributeValue(int attr, int amount);

        /**
         * Applies all the attribute modifiers to a given Being.
         */
        void applyAttributes(Being *) const;

        /**
         * Cancels all the applied modifiers to a given Being.
         * Only meant for equipment.
         */
        void cancelAttributes(Being *) const;

    private:
        std::vector< ItemModifier > mModifiers;
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
        ItemModifiers const &getModifiers() const
        { return mModifiers; }

        /**
         * Sets item modifiers.
         */
        void setModifiers(ItemModifiers const &modifiers)
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

        /**
         * Sets the sprite ID.
         */
        void setSpriteID(int spriteID)
        { mSpriteID = spriteID; }

        /**
         * Gets the sprite ID.
         */
        int getSpriteID()
        { return mSpriteID; }

    private:

        /**
         * Runs the associated script when using the item, if any.
         */
        bool runScript(Being *itemUser);

        // Item reference information
        unsigned short mDatabaseID;
        unsigned short mSpriteID; /**< The sprite that should be shown to the character */
        unsigned char mType;     /**< Type: usable, equipment. */
        unsigned short mWeight;  /**< Weight of the item. */
        unsigned short mCost;    /**< Unit cost the item. */
        unsigned short mMaxPerSlot; /**< Max item amount per slot in inventory. */
        std::string mScriptName; /**< Item script. */
        ItemModifiers mModifiers; /**< Item modifiers. */
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
