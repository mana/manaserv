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
 * $Id$
 */

#ifndef _TMWSERV_ABSTRACTCHARACTERDATA
#define _TMWSERV_ABSTRACTCHARACTERDATA

#include <string>

class MessageIn;
class MessageOut;
class Point;

/**
 * Gender of a Character.
 */
enum
{
    GENDER_MALE = 0,
    GENDER_FEMALE
};

/**
 * Attributes of a Character.
 */
enum
{
    ATT_STRENGTH = 0,
    ATT_AGILITY,
    ATT_VITALITY,
    ATT_INTELLIGENCE,
    ATT_DEXTERITY,
    ATT_LUCK,
    NB_ATTRIBUTES
};

/**
 * Numbers of inventory slots
 */
enum
{
    EQUIPMENT_SLOTS = 11,
    INVENTORY_SLOTS = 50
};

/**
 * Structure representing an item stored in the inventory.
 */
struct InventoryItem
{
    unsigned short itemClassId;
    unsigned char numberOfItemsInSlot;
    bool isEquiped;
};

class AbstractCharacterData
{
    public:
        /**
         * Stores data into a outgoing message.
         */
        void serialize(MessageOut &) const;

        /**
         * Restores data from a incomming message.
         */
        void deserialize(MessageIn &);

    protected:
        /**
         * Get and set methods
         */

        /** Gets the database id of the character. */
        virtual int
        getDatabaseID() const = 0;

        /** Sets the database id of the character. */
        virtual void
        setDatabaseID(int id) = 0;

        /** Gets the name of the character. */
        virtual std::string const &
        getName() const = 0;

        /** Sets the name of the character. */
        virtual void
        setName(const std::string&) = 0;

        /** Gets the gender of the character (male or female). */
        virtual int
        getGender() const = 0;

        /** Sets the gender of the character (male or female). */
        virtual void
        setGender(int gender) = 0;

        /** Gets the hairstyle of the character. */
        virtual int
        getHairStyle() const = 0;

        /** Sets the hairstyle of the character. */
        virtual void
        setHairStyle(int style) = 0;

        /** Gets the haircolor of the character. */
        virtual int
        getHairColor() const = 0;

        /** Sets the haircolor of the character. */
        virtual void
        setHairColor(int color) = 0;

        /** Gets the level of the character. */
        virtual int
        getLevel() const = 0;

        /** Sets the level of the character. */
        virtual void
        setLevel(int level) = 0;

        /** Gets the amount of money the character has. */
        virtual int
        getMoney() const = 0;

        /** Sets the amount of money the character has. */
        virtual void
        setMoney(int amount) = 0;

        /** Gets the value of an attribute of the character. */
        virtual unsigned short
        getAttribute(int attributeNumber) const = 0;

        /** Sets the value of an attribute of the character. */
        virtual void
        setAttribute(int attributeNumber, int value) = 0;

        /** Gets the Id of the map that the character is on. */
        virtual int
        getMapId() const = 0;

        /** Sets the Id of the map that the character is on. */
        virtual void
        setMapId(int mapId) = 0;

        /** Gets the position of the character on the map. */
        virtual Point const &
        getPos() const = 0;

        /** Sets the position of the character on the map. */
        virtual void
        setPos(const Point &p) = 0;

        /**
         * The access functions for inventory
         */

        /** Returns the number of inventory items. */
        virtual int
        getNumberOfInventoryItems() const = 0;

        /** Returns a reference to the item in inventory at slot. */
        virtual InventoryItem const &
        getInventoryItem(unsigned short slot) const = 0;

        /** Clears the inventory, in preperation for an update. */
        virtual void
        clearInventory() = 0;

        /** Adds an inventory item to the inventory. */
        virtual void
        addItemToInventory(const InventoryItem& item) = 0;
};

#endif
