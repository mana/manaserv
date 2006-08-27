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

/**
 * Raw statistics of a Player.
 */
enum {
    STAT_STR = 0,
    STAT_AGI,
    STAT_VIT,
    STAT_INT,
    STAT_DEX,
    STAT_LUK,
    NB_RSTAT
};

/**
 * Structure types for the raw statistics of a Player.
 */
struct RawStatistics
{
    unsigned short stats[NB_RSTAT];
};

/**
 * Computed statistics of a Being.
 */
enum {
    STAT_HEA = 0,
    STAT_ATT,
    STAT_DEF,
    STAT_MAG,
    STAT_ACC,
    STAT_SPD,
    NB_CSTAT
};

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
class Being : public MovingObject
{
    public:
        /**
         * Proxy constructor.
         */
        Being(int type, int id)
          : MovingObject(type, id)
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

/**
 * Type definition for a smart pointer to Being.
 */
typedef utils::CountedPtr<Being> BeingPtr;

/**
 * Type definition for a list of Beings.
 */
typedef std::vector<BeingPtr> Beings;

#endif // _TMWSERV_BEING_H_
