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

#ifndef _TMWSERV_CHARACTERDATA
#define _TMWSERV_CHARACTERDATA

#include "abstractcharacterdata.hpp"
#include "defines.h"
#include <string>
#include <vector>

#include "utils/countedptr.h"

#include "point.h"

class MessageIn;

class CharacterData: public AbstractCharacterData
{
    public:

        CharacterData(std::string const &name, int id = -1);

        /**
         * Constructor used for creating a character from a serialised message.
         */
        CharacterData(MessageIn & msg);

        /**
         * Get and set methods
         */

        /** Gets the database id of the character. */
        int
        getDatabaseID() const { return mDatabaseID; }

        /** Sets the database id of the character. */
        void
        setDatabaseID(int id) { mDatabaseID = id; }

        /** Gets the account id of the account the character belongs to. */
        int
        getAccountID() const { return mAccountID; }

        /** Sets the account id of the account the character belongs to. */
        void
        setAccountID(int id) { mAccountID = id; }

        /** Gets the name of the character. */
        std::string const &
        getName() const { return mName; }

        /** Sets the name of the character. */
        void
        setName(const std::string& name) { mName = name; }

        /** Gets the gender of the character (male / female). */
        int
        getGender() const { return mGender; }

        /** Sets the gender of the character (male / female). */
        void
        setGender(int gender) { mGender = gender; }

        /** Gets the hairstyle of the character. */
        int
        getHairStyle() const { return mHairStyle; }

        /** Sets the hairstyle of the character. */
        void
        setHairStyle(int style) { mHairStyle = style; }

        /** Gets the haircolor of the character. */
        int
        getHairColor() const { return mHairColor; }

        /** Sets the haircolor of the character. */
        void
        setHairColor(int color) { mHairColor = color; }

        /** Gets the level of the character. */
        int
        getLevel() const { return mLevel; }

        /** Sets the level of the character. */
        void
        setLevel(int level) { mLevel = level; }

        /** Gets the amount of money the character has. */
        int
        getMoney() const { return mMoney; }

        /** Sets the amount of money the character has. */
        void
        setMoney(int amount) { mMoney = amount; }

        /** Gets the value of a base attribute of the character. */
        unsigned short
        getBaseAttribute(int attributeNumber) const
        { return mBaseAttributes[attributeNumber]; }

        /** Sets the value of a base attribute of the character. */
        void
        setBaseAttribute(int attributeNumber, int value)
        { mBaseAttributes[attributeNumber] = value; }

        /** Gets the Id of the map that the character is on. */
        int
        getMapId() const { return mMapId; }

        /** Sets the Id of the map that the character is on. */
        void
        setMapId(int mapId) { mMapId = mapId; }

        /** Gets the position of the character on the map. */
        Point const &
        getPosition() const { return mPos; }

        /** Sets the position of the character on the map. */
        void
        setPosition(const Point &p) { mPos = p; }

        /** Returns the number of inventory items. */
        int
        getNumberOfInventoryItems() const { return mInventory.size(); }

        /** Returns a reference to the item in inventory at slot. */
        InventoryItem const &
        getInventoryItem(unsigned short slot) const;

        /** Clears the inventory, in preperation for an update. */
        void
        clearInventory() { mInventory.clear(); }

        /** Adds an inventory item to the inventory. */
        void
        addItemToInventory(const InventoryItem& item);

    private:
        CharacterData(CharacterData const &);
        CharacterData &operator=(CharacterData const &);

        int mDatabaseID;          //!< Character database ID.
                                  //!< (-1) if not set yet.
        int mAccountID;           //!< Account ID of the account the character
                                  //!< belongs to. (-1) if not set yet.
        std::string mName;        //!< Name of the character.
        unsigned char mGender;    //!< Gender of the being.
        unsigned char mHairStyle; //!< Hair Style of the being.
        unsigned char mHairColor; //!< Hair Color of the being.
        unsigned char mLevel;     //!< Level of the being.
        unsigned int mMoney;      //!< Wealth of the being.
        unsigned short mBaseAttributes[NB_BASE_ATTRIBUTES]; //!< The attributes of the
                                                   //!< character.
        unsigned short mMapId;    //!< Map the being is on.
        Point mPos;               //!< Position the being is at.
        std::vector< InventoryItem > mInventory; //!< All the possesions of
                                                 //!< the character.
};

//Utility typedefs
/**
 * Type definition for a smart pointer to CharacterData.
 */
typedef utils::CountedPtr< CharacterData > CharacterPtr;

/**
 * Type definition for a list of Characters.
 */
typedef std::vector< CharacterPtr > Characters;

#endif
