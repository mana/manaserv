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

#include <list>
#include <string>
#include <vector>

#include "defines.h"
#include "game-server/object.hpp"

class MapComposite;

/**
 * Element attribute for beings, actors and items.
 */
enum
{
    ELEMENT_NEUTRAL = 0,
    ELEMENT_FIRE,
    ELEMENT_WATER,
    ELEMENT_EARTH,
    ELEMENT_AIR,
    ELEMENT_SACRED,
    ELEMENT_DEATH
};

/**
 * States attribute for beings, and actors.
 * States can be multiple for the same being.
 */
struct BeingState
{
    bool STATE_NORMAL;
    bool STATE_POISONED;
    bool STATE_STONED;
    bool STATE_STUNNED;
    bool STATE_SLOWED;
    bool STATE_TIRED;
    bool STATE_MAD;
    bool STATE_BERSERK;
    bool STATE_HASTED;
    bool STATE_FLOATING;
};

/**
 * Beings and actors directions
 */
enum
{
    DIRECTION_DOWN = 1,
    DIRECTION_UP,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
};


/**
 * Computed statistics of a Being.
 */
enum
{
    STAT_HEAT = 0,
    STAT_ATTACK,
    STAT_DEFENCE,
    STAT_MAGIC,
    STAT_ACCURACY,
    STAT_SPEED,
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
 * Placeholder for a more complex damage structure
 */
typedef unsigned short Damage;

/**
 * Type definition for a list of hits
 */
typedef std::list<unsigned int> Hits;

/**
 * Generic Being (living object).
 * Used for players & monsters (all animated objects).
 */
class Being : public MovingObject
{
    public:
        /**
         * Moves enum for beings and actors for others players vision.
         * WARNING: Has to be in sync with the same enum in the Being class
         * of the client!
         */
        enum Action {
            STAND,
            WALK,
            ATTACK,
            SIT,
            DEAD,
            HURT
        };
        /**
         * Proxy constructor.
         */
        Being(int type, int id)
          : MovingObject(type, id),
            mAction(STAND)
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

        /**
         * sets the hit points
         */
        void setHitpoints(int hp)
        { mHitpoints = hp; }

        /**
         * Takes a damage structure, computes the real damage based on the
         * stats, deducts the result from the hitpoints and adds the result to
         * the HitsTaken list.
         */
        void damage(Damage);

        /**
         * Kills the being
         */
        virtual void die();

        /**
         * Gets the damage list.
         */
        Hits const &getHitsTaken() const
        { return mHitsTaken; }

        /**
         * Clears the damage list.
         */
        void clearHitsTaken()
        { mHitsTaken.clear(); }

        /**
         * Performs an attack.
         */
        void performAttack(MapComposite *);

        /**
         * Sets the current action.
         */
        virtual void setAction(Action action);

        virtual Action getAction() const
        { return mAction; }

        /**
         * Moves the being toward its destination.
         */
        virtual void move();

    protected:
        int mHitpoints; /**< Hitpoints of the being */
        Action mAction;

    private:
        Being(Being const &rhs);
        Being &operator=(Being const &rhs);

        Statistics mStats; /**< stats modifiers or computed stats */

        Hits mHitsTaken; /**< List of punches taken since last update */
};

#endif // _TMWSERV_BEING_H_
