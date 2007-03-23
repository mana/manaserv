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

#ifndef _TMWSERV_CHARACTER_HPP_
#define _TMWSERV_CHARACTER_HPP_

#include "abstractcharacterdata.hpp"
#include "game-server/being.hpp"

#include <string>
#include <vector>

class GameClient;
class MessageIn;
class Point;


struct Possessions
{
    unsigned short equipment[EQUIPMENT_SLOTS];
    std::vector< InventoryItem > inventory;
};

/**
 * The representation of a player's character in the game world.
 */
class Character : public Being, public AbstractCharacterData
{
    public:

        /**
         * Utility constructor for creating a Character from a received
         * characterdata message.
         */
        Character(MessageIn &msg);

        /**
         * Updates the internal status.
         */
        void update();

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

        /**
         * Gets a reference on the possession.
         * Used in the current Inventory class
         */
        Possessions &getPossessions()
        { return mPossessions; }


        /*
         * Character data:
         * Get and set methods
         */

        /** Gets the database id of the character. */
        int
        getDatabaseID() const
        { return mDatabaseID; }

        /** Sets the database id of the character. */
        void
        setDatabaseID(int id)
        { mDatabaseID = id; }

        /** Gets the name of the character. */
        std::string const &
        getName() const
        { return mName; }

        /** Sets the name of the character. */
        void
        setName(const std::string& name)
        { mName = name; }

        /** Gets the gender of the character (male or female). */
        int
        getGender() const
        { return mGender;}

        /** Sets the gender of the character (male or female). */
        void
        setGender(int gender)
        { mGender = gender; }

        /** Gets the hairstyle of the character. */
        int
        getHairStyle() const
        { return mHairStyle; }

        /** Sets the hairstyle of the character. */
        void
        setHairStyle(int style)
        { mHairStyle = style; }

        /** Gets the haircolor of the character. */
        int
        getHairColor() const
        { return mHairColor; }

        /** Sets the haircolor of the character. */
        void
        setHairColor(int color)
        { mHairColor = color; }

        /** Gets the level of the character. */
        int
        getLevel() const
        { return mLevel; }

        /** Sets the level of the character. */
        void
        setLevel(int level)
        { mLevel = level; }

        /** Gets the amount of money the character has. */
        int
        getMoney() const
        { return mMoney; }

        /** Sets the amount of money the character has. */
        void
        setMoney(int amount)
        { mMoney = amount; }

        /**
         * Gets the value of an attribute of the character.
         * Inherited from Being, explicitly defined because
         * of double inheritance.
         */
        unsigned short
        getBaseAttribute(int attributeNumber) const
        { return Being::getAttribute(attributeNumber); }

        /**
         * Sets the value of an attribute of the character.
         * Inherited from Being, explicitly defined because
         * of double inheritance.
         */
        void
        setBaseAttribute(int attributeNumber, int value)
        { Being::setAttribute(attributeNumber, value); }

        /**
         * Creates a message that informs the client about the attribute
         * changes since last call.
         */
        void
        writeAttributeUpdateMessage(MessageOut &msg);

        /**
         * Gets the Id of the map that the character is on.
         * Inherited from Thing through Being, explicitly defined because
         * of double inheritance.
         */
        int
        getMapId() const
        { return Being::getMapId(); }

        /**
         * Sets the Id of the map that the character is on.
         * Inherited from Thing through Being, explicitly defined because
         * of double inheritance.
         */
        void
        setMapId(int mapId)
        { Being::setMapId(mapId); }

        /**
         * Gets the position of the character on the map.
         * Inherited from Object through Being, explicitly defined because
         * of double inheritance.
         */
        Point const &
        getPosition() const
        { return Being::getPosition(); }

        /**
         * Sets the position of the character on the map.
         * Inherited from Object through Being, explicitly defined because
         * of double inheritance.
         */
        void
        setPosition(const Point &p)
        { Being::setPosition(p); }

        /**
         * The access functions for inventory
         *
         * Currently not implemented
         */

        /**
         * Returns the number of inventory items.
         * (items don't have to be unique)
         * TODO: maybe renaming to NumberOfFilledSlots would be better.
         */
        int
        getNumberOfInventoryItems() const;

        /**
         * Returns a reference to the item in inventory at slot.
         * TODO: Keep this consistent with whatever is chosen for
         *       getNumberOfInventoryItems.
         */
        InventoryItem const &
        getInventoryItem(unsigned short slot) const;

        /** Clears the inventory, in preperation for an update. */
        void
        clearInventory();

        /** Adds an inventory item to the inventory. */
        void
        addItemToInventory(const InventoryItem& item);

    protected:
        /**
         * Calculates all derived attributes
         */
        void calculateDerivedAttributes();

        /**
         * Gets the stats of the currently equipped weapon that are relevant
         * for damage calculation
         */
        virtual WeaponStats getWeaponStats();

    private:
        Character(Character const &);
        Character &operator=(Character const &);

        GameClient *mClient;   /**< Client computer. */

        /** Atributes as the client should currently know them. */
        std::vector<unsigned short> mOldAttributes;

        /**
         * true when one or more attributes might have changed since the
         * client has been updated about them.
         */
        bool mAttributesChanged;

        int mDatabaseID;             /**< Character's database ID. */
        std::string mName;           /**< Name of the character. */
        unsigned char mGender;       /**< Gender of the character. */
        unsigned char mHairStyle;    /**< Hair Style of the character. */
        unsigned char mHairColor;    /**< Hair Color of the character. */
        unsigned char mLevel;        /**< Level of the character. */
        unsigned int mMoney;         /**< Wealth of the being. */

        Possessions mPossessions;    /**< Possesssions of the character. */
};

#endif // _TMWSERV_CHARACTER_HPP_
