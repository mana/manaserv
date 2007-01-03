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

#ifndef _TMWSERV_PLAYERDATA
#define _TMWSERV_PLAYERDATA

#include <string>
#include <vector>

#include "point.h"
#include "utils/countedptr.h"

/**
 * Gender of a Player.
 */
enum
{
    GENDER_MALE = 0,
    GENDER_FEMALE
};

/**
 * Raw statistics of a Player.
 */
enum
{
    STAT_STRENGTH = 0,
    STAT_AGILITY,
    STAT_VITALITY,
    STAT_INTELLIGENCE,
    STAT_DEXTERITY,
    STAT_LUCK,
    NB_RSTAT
};

/**
 * Structure types for the raw statistics of a Player.
 */
struct RawStatistics
{
    unsigned short stats[NB_RSTAT];
};


class PlayerData
{
    public:

        PlayerData(std::string const &name, int id = -1)
          : mDatabaseID(id),
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
        void setHairStyle(int style)
        { mHairStyle = style; }

        /**
         * Gets the hair style.
         *
         * @return the hair style value.
         */
        int getHairStyle() const
        { return mHairStyle; }

        /**
         * Sets the hair color.
         *
         * @param color the new hair color.
         */
        void setHairColor(int color)
        { mHairColor = color; }

        /**
         * Gets the hair color.
         *
         * @return the hair color value.
         */
        int getHairColor() const
        { return mHairColor; }

        /**
         * Sets the gender.
         *
         * @param gender the new gender.
         */
        void setGender(int gender)
        { mGender = gender; }

        /**
         * Gets the gender.
         *
         * @return the gender.
         */
        int getGender() const
        { return mGender; }

        /**
         * Sets the level.
         *
         * @param level the new level.
         */
        void setLevel(int level)
        { mLevel = level; }

        /**
         * Gets the level.
         *
         * @return the level.
         */
        int getLevel() const
        { return mLevel; }

        /**
         * Sets the money.
         *
         * @param amount the new amount.
         */
        void setMoney(int amount)
        { mMoney = amount; }

        /**
         * Gets the amount of money.
         *
         * @return the amount of money.
         */
        int getMoney() const
        { return mMoney; }

        /**
         * Sets a raw statistic.
         *
         * @param numStat the statistic number.
         * @param value the new value.
         */
        void setRawStat(int numStat, int value)
        { mRawStats.stats[numStat] = value; }

        /**
         * Gets a raw statistic.
         *
         * @param numStat the statistic number.
         * @return the statistic value.
         */
        int getRawStat(int numStat)
        { return mRawStats.stats[numStat]; }

        /**
         * Gets database ID.
         *
         * @return the database ID, a negative number if none yet.
         */
        int getDatabaseID() const
        { return mDatabaseID; }

        /**
         * Sets database ID.
         * The object shall not have any ID yet.
         */
        void setDatabaseID(int id)
        { mDatabaseID = id; }

        /**
         * Gets the map this thing is located on.
         *
         * @return ID of map.
         */
        int getMap() const
        { return mMapId; }

        /**
         * Sets the map this thing is located on.
         */
        void setMap(int mapId)
        { mMapId = mapId; }

        /**
         * Sets the coordinates.
         *
         * @param p the coordinates.
         */
        void setPos(const Point &p)
        { mPos = p; }

        /**
         * Gets the coordinates.
         *
         * @return the coordinates.
         */
        Point const &getPos() const
        { return mPos; }

    private:
        PlayerData(PlayerData const &);
        PlayerData &operator=(PlayerData const &);

        int mDatabaseID;          /**< Player database ID. */
        std::string mName;        /**< Name of the being. */
        unsigned char mGender;    /**< Gender of the being. */
        unsigned char mHairStyle; /**< Hair Style of the being. */
        unsigned char mHairColor; /**< Hair Color of the being. */
        unsigned char mLevel;     /**< Level of the being. */
        unsigned short mMapId;    /**< Map the being is on. */
        Point mPos;               /**< Position the being is at. */
        unsigned int mMoney;      /**< Wealth of the being. */
        RawStatistics mRawStats;  /**< Raw statistics of the being. */
};

/**
 * Type definition for a smart pointer to PlayerData.
 */
typedef utils::CountedPtr< PlayerData > PlayerPtr;

/**
 * Type definition for a list of Players.
 */
typedef std::vector< PlayerPtr > Players;

#endif
