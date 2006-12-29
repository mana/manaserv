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

#ifndef _TMWSERV_PLAYER_H_
#define _TMWSERV_PLAYER_H_

#include <string>
#include <vector>

#include "being.h"
#include "defines.h"
#include "inventory.h"
#include "utils/countedptr.h"

class GameClient;

class Player : public Being
{
    public:

        Player(std::string const &name, int id = -1)
          : Being(OBJECT_PLAYER, 65535),
            mDatabaseID(id),
            mName(name),
            mClient(NULL),
            mIsAttacking(false)
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
        void setGender(Gender gender)
        { mGender = gender; }

        /**
         * Gets the gender.
         *
         * @return the gender.
         */
        Gender getGender() const
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
        setInventory(const Inventory &inven);

        /**
         * Adds item with ID to inventory.
         *
         * @return Item add success/failure
         */
        bool
        addItem(unsigned int itemId, unsigned char amount = 1);

        /**
         * Removes item with ID from inventory.
         *
         * @return Item delete success/failure
         */
        bool
        removeItem(unsigned int itemId, unsigned char amount = 0);

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
        equip(unsigned char slot);

        /**
         * Un-equips item.
         *
         * @return Un-equip success/failure
         */
        bool
        unequip(unsigned char slot);

        /**
         * Set attacking state
         **/
        void setAttacking(bool isAttacking)
        { mIsAttacking = isAttacking; }

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
         * Gets client computer.
         */
        GameClient *getClient() const
        { return mClient; }

    private:
        Player(Player const &);
        Player &operator=(Player const &);

        int mDatabaseID;          /**< Player database ID (unique with respect to its type) */
        std::string mName;        /**< name of the being */
        GameClient *mClient;      /**< client computer, directly set by GameClient */
        Gender mGender;           /**< gender of the being */
        unsigned char mHairStyle; /**< Hair Style of the being */
        unsigned char mHairColor; /**< Hair Color of the being */
        unsigned char mLevel;     /**< level of the being */
        unsigned int mMoney;      /**< wealth of the being */
        RawStatistics mRawStats;  /**< raw stats of the being */

        Inventory inventory;    /**< Player inventory and Equipment */

        bool mIsAttacking; /**< attacking state */

        friend class GameClient;
};

/**
 * Type definition for a smart pointer to Player.
 */
typedef utils::CountedPtr<Player> PlayerPtr;

/**
 * Type definition for a list of Players.
 */
typedef std::vector<PlayerPtr> Players;

#endif // _TMWSERV_PLAYER_H_
