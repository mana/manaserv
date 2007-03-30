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
#include "game-server/movingobject.hpp"

class Being;
class MapComposite;

/**
 * Derived attributes of a Being.
 */
enum
{

};

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
 * Structure that describes the severity and nature of an attack a being can
 * be hit by.
 */
struct Damage
{
    int value;
    int piercing;
    Element element;
    Damagetype type;
    Being *source;
};

/**
 * Structure that holds weapon stats that are relevant for damage calculation
 */
struct WeaponStats
{
    int piercing;
    Element element;
    int skill;
};

/**
 * Type definition for a list of hits
 */
typedef std::list<unsigned int> Hits;


/**
 * Generic Being (living object).
 * Used for characters & monsters (all animated objects).
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
        Being(int type, int id);

        ~Being();

        /**
         * Creates a damage structure for a normal melee attack based on the
         * current being stats and equipment.
         */
        Damage getPhysicalAttackDamage();

        /**
         * Sets the hit points to maximum
         */
        void fillHitpoints()
        { mHitpoints = getAttribute(DERIVED_ATTR_HP_MAXIMUM); }

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

        /**
         * Sets the current action.
         */
        virtual Action getAction() const
        { return mAction; }

        /**
         * Moves the being toward its destination.
         */
        virtual void move();

        /**
         * Sets an attribute (doesn't work on derived attributes)
         */
        void setAttribute(int attributeNumber, unsigned short value)
        {
            mAttributes.at(attributeNumber) = value;
            calculateDerivedAttributes();
        }

        /**
         * Gets an attribute.
         */
        unsigned short getAttribute(int attributeNumber) const
        { return mAttributes.at(attributeNumber); }

    protected:
        /**
         * Calculates all derived attributes of a beings
         */
        void calculateDerivedAttributes();

        /**
         * Gets the stats of the currently equipped weapon that are relevant
         * for damage calculation
         */
        virtual WeaponStats getWeaponStats();

        int mHitpoints; /**< Hitpoints of the being */
        Action mAction;

        std::vector<unsigned short> mAttributes;

    private:
        Being(Being const &rhs);
        Being &operator=(Being const &rhs);


        Hits mHitsTaken; /**< List of punches taken since last update */
};

#endif // _TMWSERV_BEING_H_
