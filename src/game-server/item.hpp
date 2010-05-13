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

#ifndef ITEM_HPP
#define ITEM_HPP

#include <vector>

#include "game-server/actor.hpp"

class Being;
class Script;

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

ItemType itemTypeFromString (const std::string &name);

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
        ItemClass(int id, ItemType type, Script *s = NULL)
          : mScript(NULL), mDatabaseID(id), mType(type), mAttackRange(0)
        {}

        ~ItemClass();

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
        const ItemModifiers &getModifiers() const
        { return mModifiers; }

        /**
         * Sets item modifiers.
         */
        void setModifiers(const ItemModifiers &modifiers)
        { mModifiers = modifiers; }

        /**
         * Gets database ID.
         */
        int getDatabaseID() const
        { return mDatabaseID; }

        /**
         * Sets the sprite ID.
         */
        void setSpriteID(int spriteID)
        { mSpriteID = spriteID; }

        /**
         * Gets the sprite ID.
         */
        int getSpriteID() const
        { return mSpriteID; }

        /**
         * Sets the script that is to be used
         */
        void setScript(Script *s)
        { mScript = s; }

        /**
         * Set attack range (only needed when the item is a weapon)
         */
        void setAttackRange(unsigned range) { mAttackRange = range; }

        /**
         * Gets attack zone of weapon (returns NULL for non-weapon items)
         */
        const unsigned getAttackRange() const
        { return mAttackRange ; }


    private:
        Script *mScript;          /**< Script for using items */

        unsigned short mDatabaseID; /**< Item reference information */
        /** The sprite that should be shown to the character */
        unsigned short mSpriteID;
        ItemType mType;           /**< Type: usable, equipment etc. */
        unsigned short mWeight;   /**< Weight of the item. */
        unsigned short mCost;     /**< Unit cost the item. */
        /** Max item amount per slot in inventory. */
        unsigned short mMaxPerSlot;

        ItemModifiers mModifiers; /**< Item modifiers. */
        unsigned mAttackRange;  /**< Attack range when used as a weapon */
};

/**
* Class for an item stack laying on the floor in the game world
*/

class Item : public Actor
{
    public:
        Item(ItemClass *type, int amount);

        ItemClass *getItemClass() const
        { return mType; }

        int getAmount() const
        { return mAmount; }

        virtual void update();

    private:
        ItemClass *mType;
        unsigned char mAmount;
        int mLifetime;
};

#endif
