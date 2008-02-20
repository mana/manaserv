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

#include <vector>

#include "game-server/object.hpp"

class Being;

/**
 * Enumeration of available Item types.
 */
enum ItemType
{
    ITEM_UNUSABLE = 0,
    ITEM_USABLE, //                     1
    ITEM_EQUIPMENT_ONE_HAND_WEAPON, //  2
    ITEM_EQUIPMENT_TWO_HANDS_WEAPON,//  3
    ITEM_EQUIPMENT_TORSO,//             4
    ITEM_EQUIPMENT_ARMS,//              5
    ITEM_EQUIPMENT_HEAD,//              6
    ITEM_EQUIPMENT_LEGS,//              7
    ITEM_EQUIPMENT_SHIELD,//            8
    ITEM_EQUIPMENT_RING,//              9
    ITEM_EQUIPMENT_NECKLACE,//         10
    ITEM_EQUIPMENT_FEET,//             11
    ITEM_EQUIPMENT_AMMO,//              12
    ITEM_HAIRSPRITE,
    ITEM_RACESPRITE,
    ITEM_UNKNOWN
};

/**
 * Enumeration of available weapon's types.
 */
enum WeaponType
{
    WPNTYPE_NONE = 0,
    WPNTYPE_KNIFE,
    WPNTYPE_SWORD,
    WPNTYPE_POLEARM,
    WPNTYPE_STAFF,
    WPNTYPE_WHIP,
    WPNTYPE_BOW,
    WPNTYPE_SHOOTING,
    WPNTYPE_MACE,
    WPNTYPE_AXE,
    WPNTYPE_THROWN
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
        ItemClass(int id, ItemType type)
          : mDatabaseID(id), mType(type)
        {}

        /**
         * Applies the modifiers of an item to a given user.
         * @return true if the item was sucessfully used and should be removed.
         */
        bool use(Being *itemUser);

        /**
         * Gets item type.
         */
        ItemType getType() const
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

        // Item reference information
        unsigned short mDatabaseID;
        unsigned short mSpriteID; /**< The sprite that should be shown to the character */
        ItemType mType;     /**< Type: usable, equipment etc. */
        unsigned short mWeight;  /**< Weight of the item. */
        unsigned short mCost;    /**< Unit cost the item. */
        unsigned short mMaxPerSlot; /**< Max item amount per slot in inventory. */
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
