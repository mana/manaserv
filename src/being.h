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
#include "object.h"
#include "utils/countedptr.h"

class Controller;

/**
 * Element attribute for beings, actors and items.
 */
typedef enum {
    ELEMENT_NEUTRAL = 0,
    ELEMENT_FIRE,
    ELEMENT_WATER,
    ELEMENT_EARTH,
    ELEMENT_AIR,
    ELEMENT_SACRED,
    ELEMENT_DEATH
} Element;

/**
 * States attribute for beings, and actors.
 * States can be multiple for the same being.
 */
struct BeingState {
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
 * Moves enum for beings and actors for others players vision.
 */
typedef enum {
    ACTION_DEFAULT = 0,
    ACTION_STAND,
    ACTION_WALK,
    ACTION_RUN,
    ACTION_JUMP,
    ACTION_CRAWL,
    ACTION_ATTACK,
    ACTION_ATTACK_SWING,
    ACTION_ATTACK_STAB,
    ACTION_ATTACK_BOW,
    ACTION_ATTACK_THROW,
    ACTION_CAST_MAGIC,
    ACTION_USE_ITEM,
    ACTION_SIT,
    ACTION_SLEEP,
    ACTION_HURT,
    ACTION_DEAD,
    ACTION_INVALID
} SpriteAction;

/**
 * Beings and actors directions
 */
typedef enum {
    DIRECTION_NORTH,
    DIRECTION_NORTHWEST,
    DIRECTION_NORTHEAST,
    DIRECTION_WEST,
    DIRECTION_EAST,
    DIRECTION_SOUTH,
    DIRECTION_SOUTHWEST,
    DIRECTION_SOUTHEAST
} SpriteDirection;

/**
 * Raw statistics of a Player.
 */
enum {
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


/**
 * Computed statistics of a Being.
 */
enum {
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

        /**
         * Takes a damage structure, computes the real damage based on the
         * stats, deducts the result from the hitpoints and adds the result to
         * the HitsTaken list
         */
        virtual void damage(Damage);

        /**
         * Get the damage list
         */
        Hits getHitsTaken() const
        { return mHitsTaken; }

        /**
         * Clears the hit list.
         * When a controller is set, updates the controller.
         */
        virtual void
        update();

        virtual void
        performAttack(MapComposite*);

        /**
         * Notification that this being is now possessed by the given
         * controller. This means that events regarding what happens to this
         * being should be send there.
         */
        void
        possessedBy(Controller *controller)
        { mController = controller; }

    private:
        Being(Being const &rhs);
        Being &operator=(Being const &rhs);

        Statistics mStats; /**< stats modifiers or computed stats */
        Controller *mController;

        int mHitpoints; /**< Hitpoints of the being */

        Hits mHitsTaken; /**< List of punches taken since last update */
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
