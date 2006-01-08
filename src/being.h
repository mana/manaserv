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


#ifndef _TMWSERV_BEING_H_
#define _TMWSERV_BEING_H_


#include <string>
#include <vector>

#include "defines.h"
#include "object.h"
#include "utils/countedptr.h"

const unsigned int MAX_EQUIP_SLOTS = 5; /**< Maximum number of equipped slots */

namespace tmwserv
{


struct PATH_NODE {
    /**
     * Constructor.
     */
    PATH_NODE(unsigned short x, unsigned short y);

    unsigned short x, y;
};

/**
 * Structure type for the raw statistics of a Being.
 */
struct RawStatistics
{
    unsigned short strength;
    unsigned short agility;
    unsigned short vitality;
    unsigned short intelligence;
    unsigned short dexterity;
    unsigned short luck;
};


/**
 * Generic Being (living object).
 * Used for players & monsters (all animated objects).
 */
class Being: public Object
{
    public:
        /**
         * Constructor.
         */
        Being(const std::string& name,
              const Genders gender,
              const unsigned short hairStyle,
              const unsigned short hairColor,
              const unsigned short level,
              const unsigned int money,
              const RawStatistics& stats);

        /**
         * Destructor.
         */
        ~Being(void)
            throw();

        /**
         * Gets the name.
         *
         * @return the name.
         */
        const std::string&
        getName(void) const;

        /**
         * Gets the hair Style.
         *
         * @return the Hair style value.
         */
        unsigned short
        getHairStyle(void) const;

        /**
         * Gets the Hair Color.
         *
         * @return the Hair Color value.
         */
        unsigned short
        getHairColor(void) const;

        /**
         * Gets the gender.
         *
         * @return the gender.
         */
        Genders
        getGender(void) const;


        /**
         * Sets the level.
         *
         * @param level the new level.
         */
        void
        setLevel(const unsigned short level);

        /**
         * Gets the level.
         *
         * @return the level.
         */
        unsigned short
        getLevel(void) const;

        /**
         * Sets the money.
         *
         * @param amount the new amount.
         */
        void
        setMoney(const unsigned int amount);

        /**
         * Gets the amount of money.
         *
         * @return the amount of money.
         */
        unsigned int
        getMoney(void) const;

        /**
         * Sets the strength.
         *
         * @param strength the new strength.
         */
        void
        setStrength(const unsigned short strength);

        /**
         * Gets the strength.
         *
         * @return the strength.
         */
        unsigned short
        getStrength(void) const;

        /**
         * Sets the agility.
         *
         * @param agility the new agility.
         */
        void
        setAgility(const unsigned short agility);

        /**
         * Gets the agility.
         *
         * @return the agility.
         */
        unsigned short
        getAgility(void) const;

        /**
         * Sets the vitality.
         *
         * @param vitality the new vitality.
         */
        void
        setVitality(const unsigned short vitality);

        /**
         * Gets the vitality.
         *
         * @return the vitality.
         */
        unsigned short
        getVitality(void) const;

        /**
         * Sets the intelligence.
         *
         * @param intelligence the new intelligence.
         */
        void
        setIntelligence(const unsigned short intelligence);

        /**
         * Gets the intelligence.
         *
         * @return the intelligence.
         */
        unsigned short
        getIntelligence(void) const;

        /**
         * Sets the dexterity.
         *
         * @param dexterity the new dexterity.
         */
        void
        setDexterity(const unsigned short dexterity);

        /**
         * Gets the dexterity.
         *
         * @return the dexterity.
         */
        unsigned short
        getDexterity(void) const;

        /**
         * Sets the luck.
         *
         * @param luck the new luck.
         */
        void
        setLuck(const unsigned short luck);

        /**
         * Gets the luck.
         *
         * @return the luck.
         */
        unsigned short
        getLuck(void) const;

        /**
         * Sets the raw statistics.
         *
         * @param stats the new raw statistics.
         */
        void
        setRawStatistics(const RawStatistics& stats);

        /**
         * Gets the raw statistics.
         *
         * @return the raw statistics.
         */
        RawStatistics&
        getRawStatistics(void);

        /**
         * Updates the internal status.
         */
        void
        update(void);

        /**
         * Sets inventory.
         */
        void
        setInventory(const std::vector<unsigned int> &inven);

        /**
         * Adds item with ID to inventory.
         *
         * @return Item add success/failure
         */
        bool
        addInventory(unsigned int itemId);

        /**
         * Removes item with ID from inventory.
         *
         * @return Item delete success/failure
         */
        bool
        delInventory(unsigned int itemId);

        /**
         * Checks if character has an item.
         *
         * @return true if being has item, false otherwise
         */
        bool
        hasItem(unsigned int itemId);

        /**
         * Equips item with ID in equipment slot.
         *
         * @return Equip success/failure
         */
        bool
        equip(unsigned int itemId, unsigned char slot);

        /**
         * Un-equips item.
         *
         * @return Un-equip success/failure
         */
        bool
        unequip(unsigned char slot);

    private:
        /**
         * Copy constructor.
         */
        Being(const Being& rhs);

        /**
         * Assignment operator.
         */
        Being&
        operator=(const Being& rhs);

        std::string mName;       /**< name of the being */
        Genders mGender;         /**< gender of the being */
        unsigned short mHairStyle;/**< Hair Style of the being */
        unsigned short mHairColor;/**< Hair Color of the being */
        unsigned short mLevel;   /**< level of the being */
        unsigned int mMoney;     /**< wealth of the being */
        RawStatistics mRawStats; /**< raw stats of the being */

        std::vector<unsigned int> inventory;    /**< Player inventory */
        unsigned int equipment[MAX_EQUIP_SLOTS]; /**< Equipped item ID's (from inventory) */
};


/**
 * Type definition for a smart pointer to Being.
 */
typedef utils::CountedPtr<Being> BeingPtr;


/**
 * Type definition for a list of Beings.
 */
typedef std::vector<BeingPtr> Beings;


} // namespace tmwserv


#endif // _TMWSERV_BEING_H_
