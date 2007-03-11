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

class Being;
class MapComposite;

/**
 * Element attribute for beings, actors and items.
 * Subject to change until pauan and dabe are finished with the element system.
 */
enum Element
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
 * Methods of damage calculation
 */
enum Damagetype
{
    DAMAGETYPE_PHYSICAL,
    DAMAGETYPE_MAGICAL,
    DAMAGETYPE_HAZARD,
    DAMAGETYPE_OTHER
};

/**
 * Structure describing severity and nature of an attack a being can suffer of
 */
struct Damage
{
    int value;
    int penetration;
    Element element;
    Damagetype type;
    Being *source;
};

/**
 * Type definition for a list of hits
 */
typedef std::list<unsigned int> Hits;

/**
 * Structure type for the stats of a Being.
 */
struct Stats
{
    std::vector<unsigned short> base;
    std::vector<short> absoluteModificator;
    std::vector< std::list<short> > percentModificators;
};

/**
 * Generic Being (living object).
 * Used for players & monsters (all animated objects).
 */
class Being : public MovingObject
{
    public:

        /**
         * Computed statistics of a Being.
         */
        enum Stat
        {
            STAT_HP_MAXIMUM,
            STAT_PHYSICAL_ATTACK_MINIMUM,
            STAT_PHYSICAL_ATTACK_FLUCTUATION,
            STAT_PHYSICAL_DEFENCE,
            // add new computed statistics on demand
            NB_STATS_BEING
        };

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
         * Sets a being statistic.
         *
         * @param numStat the statistic number.
         * @param value the new value.
         */
        void setBaseStat(unsigned numStat, unsigned short value)
        {   mStats.base[numStat] = value;
            calculateBaseStats();
        }

        /**
         * Adds a fixed value stat modifier
         */
        void addAbsoluteStatModifier(unsigned numStat, short value);

        /**
         * Removes a fixed value stat modifier
         */
        void removeAbsoluteStatModifier(unsigned numStat, short value);

        /**
         * Adds a multiplier stat modificator in percent
         */
        void addPercentStatModifier(unsigned numStat, short value);

        /**
         * Removes a previously added percent stat modifier.
         * Does nothing and logs a warning when no modifier with the same
         * value has been added before.
         */
        void removePercentStatModifier(unsigned numStat, short value);

        /**
         * Returns a being statistic without temporary modifiers
         *
         * @param numStat the statistic number.
         * @return the statistic value.
         */
        unsigned short getBaseStat(unsigned  stat)
        { return mStats.base.at(stat); }

        /**
         * Returns a being statistic with added temporary modifiers
         */
        unsigned short getRealStat(unsigned stat);

        /**
         * Recalculates all stats of the being that are derived from others.
         * Call whenever you change something that affects a derived stat.
         * Called automatically when you manipulate a stat using setBaseStat()
         */
        virtual void calculateBaseStats()
        { /*NOOP*/ };

        /**
         * Creates a damage structure for a normal melee attack based on the
         * current being stats and equipment.
         */
        Damage getPhysicalAttackDamage();

        /**
         * sets the hit points
         */
        void setHitpoints(unsigned hp)
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

        Stats mStats;

    private:
        Being(Being const &rhs);
        Being &operator=(Being const &rhs);

        Hits mHitsTaken; /**< List of punches taken since last update */
};

#endif // _TMWSERV_BEING_H_
