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
              const unsigned short level,
              const unsigned int money,
              const RawStatistics& stats);


        /**
         * Destructor.
         */
        ~Being(void)
            throw();


        /**
         * Get the name.
         *
         * @return the name.
         */
        const std::string&
        getName(void) const;


        /**
         * Get the gender.
         *
         * @return the gender.
         */
        Genders
        getGender(void) const;


        /**
         * Set the level.
         *
         * @param level the new level.
         */
        void
        setLevel(const unsigned short level);


        /**
         * Get the level.
         *
         * @return the level.
         */
        unsigned short
        getLevel(void) const;


        /**
         * Set the money.
         *
         * @param amount the new amount.
         */
        void
        setMoney(const unsigned int amount);


        /**
         * Get the amount of money.
         *
         * @return the amount of money.
         */
        unsigned int
        getMoney(void) const;


        /**
         * Set the strength.
         *
         * @param strength the new strength.
         */
        void
        setStrength(const unsigned short strength);


        /**
         * Get the strength.
         *
         * @return the strength.
         */
        unsigned short
        getStrength(void) const;


        /**
         * Set the agility.
         *
         * @param agility the new agility.
         */
        void
        setAgility(const unsigned short agility);


        /**
         * Get the agility.
         *
         * @return the agility.
         */
        unsigned short
        getAgility(void) const;


        /**
         * Set the vitality.
         *
         * @param vitality the new vitality.
         */
        void
        setVitality(const unsigned short vitality);


        /**
         * Get the vitality.
         *
         * @return the vitality.
         */
        unsigned short
        getVitality(void) const;


        /**
         * Set the intelligence.
         *
         * @param intelligence the new intelligence.
         */
        void
        setIntelligence(const unsigned short intelligence);


        /**
         * Get the intelligence.
         *
         * @return the intelligence.
         */
        unsigned short
        getIntelligence(void) const;


        /**
         * Set the dexterity.
         *
         * @param dexterity the new dexterity.
         */
        void
        setDexterity(const unsigned short dexterity);


        /**
         * Get the dexterity.
         *
         * @return the dexterity.
         */
        unsigned short
        getDexterity(void) const;


        /**
         * Set the luck.
         *
         * @param luck the new luck.
         */
        void
        setLuck(const unsigned short luck);


        /**
         * Get the luck.
         *
         * @return the luck.
         */
        unsigned short
        getLuck(void) const;


        /**
         * Set the raw statistics.
         *
         * @param stats the new raw statistics.
         */
        void
        setRawStatistics(const RawStatistics& stats);


        /**
         * Get the raw statistics.
         *
         * @return the raw statistics.
         */
        RawStatistics&
        getRawStatistics(void);


        /**
         * Update the internal status.
         */
        void
        update(void);

        /**
         * Set inventory
         */
        void setInventory(const std::vector<unsigned int> &inven);

        /**
         * Add item with ID to inventory
         *
         * @return Item add success/failure
         */
        bool addInventory(unsigned int itemId);

        /**
         * Remove item with ID from inventory
         *
         * @return Item delete success/failure
         */
        bool delInventory(unsigned int itemId);

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


    private:
        std::string mName;       /**< name of the being */
        Genders mGender;         /**< gender of the being */
        unsigned short mLevel;   /**< level of the being */
        unsigned int mMoney;     /**< wealth of the being */
        RawStatistics mRawStats; /**< raw stats of the being */

        std::vector<unsigned int> inventory;    /**< Player inventory */
        unsigned int equipped[MAX_EQUIP_SLOTS]; /**< Equipped item ID's (from inventory) */
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
