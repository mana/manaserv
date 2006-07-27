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

struct PATH_NODE {
    /**
     * Constructor.
     */
    PATH_NODE(unsigned short x, unsigned short y);

    unsigned short x, y;
};

/**
 * Raw statistics of a Player
 */

enum { STAT_STR = 0, STAT_AGI, STAT_VIT, STAT_INT, STAT_DEX, STAT_LUK, NB_RSTAT };

/**
 * Structure types for the raw statistics of a Player
 */

struct RawStatistics
{
    unsigned short stats[NB_RSTAT];
};

/*
 * Computed statistics of a Being
 */

enum { STAT_HEA = 0, STAT_ATT, STAT_DEF, STAT_MAG, STAT_ACC, STAT_SPD, NB_CSTAT };

/**
 * Structure type for the computed statistics of a Being.
 */
struct Statistics
{
    unsigned short stats[NB_CSTAT];
};

/**
 * Generic Being (living object).
 * Used for players & monsters (all animated objects).
 */
class Being: public MovingObject
{
    public:
        /**
         * Proxy constructor.
         */
        Being(int type)
          : MovingObject(type)
        {}

        /**
         * Sets a computed statistic.
         *
         * @param numStat the statistic number.
         * @param value the new value.
         */
        void setStat(int numStat, unsigned short value)
        { mStats.stats[numStat] = value; }

        /**
         * Gets a computed statistic.
         *
         * @param numStat the statistic number.
         * @return the statistic value.
         */
        unsigned short getStat(int numStat)
        { return mStats.stats[numStat]; }

    private:
        Being(Being const &rhs);
        Being &operator=(Being const &rhs);

        Statistics mStats; /**< stats modifiers or computed stats */
};

class Player: public Being
{
    public:

        Player(std::string const &name)
          : Being(OBJECT_PLAYER),
            mName(name)
        {}

        /**
         * Gets the name.
         *
         * @return the name.
         */
        std::string const &getName() const
        { return mName; }

        /**
         * Sets the hair style.
         *
         * @param style the new hair style.
         */
        void setHairStyle(unsigned char style)
        { mHairStyle = style; }

        /**
         * Gets the hair style.
         *
         * @return the hair style value.
         */
        unsigned char getHairStyle() const
        { return mHairStyle; }

        /**
         * Sets the hair color.
         *
         * @param color the new hair color.
         */
        void setHairColor(unsigned char color)
        { mHairColor = color; }

        /**
         * Gets the hair color.
         *
         * @return the hair color value.
         */
        unsigned char getHairColor() const
        { return mHairColor; }

        /**
         * Sets the gender.
         *
         * @param gender the new gender.
         */
        void setGender(Genders gender)
        { mGender = gender; }

        /**
         * Gets the gender.
         *
         * @return the gender.
         */
        Genders getGender() const
        { return mGender; }

        /**
         * Sets the level.
         *
         * @param level the new level.
         */
        void setLevel(unsigned char level)
        { mLevel = level; }

        /**
         * Gets the level.
         *
         * @return the level.
         */
        unsigned char getLevel() const
        { return mLevel; }

        /**
         * Sets the money.
         *
         * @param amount the new amount.
         */
        void setMoney(unsigned int amount)
        { mMoney = amount; }

        /**
         * Gets the amount of money.
         *
         * @return the amount of money.
         */
        unsigned int getMoney() const
        { return mMoney; }

        /**
         * Sets a raw statistic.
         *
         * @param numStat the statistic number.
         * @param value the new value.
         */
        void setRawStat(int numStat, unsigned short value)
        { mRawStats.stats[numStat] = value; }

        /**
         * Gets a raw statistic.
         *
         * @param numStat the statistic number.
         * @return the statistic value.
         */
        unsigned short getRawStat(int numStat)
        { return mRawStats.stats[numStat]; }

        /**
         * Updates the internal status.
         */
        void update();

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
        Player(Player const &);
        Player &operator=(Player const &);

        std::string mName;       /**< name of the being */
        Genders mGender;         /**< gender of the being */
        unsigned char mHairStyle;/**< Hair Style of the being */
        unsigned char mHairColor;/**< Hair Color of the being */
        unsigned char mLevel;    /**< level of the being */
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

/**
 * Type definition for a smart pointer to Player.
 */
typedef utils::CountedPtr<Player> PlayerPtr;

/**
 * Type definition for a list of Players.
 */
typedef std::vector<PlayerPtr> Players;

#endif // _TMWSERV_BEING_H_
