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
 */

#ifndef _TMWSERV_BEING_H_
#define _TMWSERV_BEING_H_

#include <string>
#include <vector>
#include "limits.h"

#include "defines.h"
#include "game-server/movingobject.hpp"

class Being;
class MapComposite;
class AttackZone;

/**
 * Beings and actors directions
 * Needs too match client
 */
enum Direction
{
    DIRECTION_UP = 1,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
};

/**
 * Methods of damage calculation
 */
enum
{
    DAMAGE_PHYSICAL = 0,
    DAMAGE_MAGICAL,
    DAMAGE_OTHER
};

/**
 * Structure that describes the severity and nature of an attack a being can
 * be hit by.
 */
struct Damage
{
    unsigned short base;   /**< Base amount of damage. */
    unsigned short delta;  /**< Additional damage when lucky. */
    unsigned short cth;    /**< Chance to hit. Opposes the evade attribute. */
    unsigned char element; /**< Elemental damage. */
    unsigned char type;    /**< Damage type: Physical or magical? */
    size_t usedSkill;      /**< Skill used by source (needed for exp calculation) */
};

/**
 * Holds the base value of an attribute and the sum of all its modifiers.
 * While base + mod may be negative, the modified attribute is not.
 */
struct Attribute
{
    unsigned short base;
    short mod;
};

struct AttributeModifier
{
    /**< Number of ticks (0 means permanent, e.g. equipment). */
    unsigned short duration;
    short value;         /**< Positive or negative amount. */
    unsigned char attr;  /**< Attribute to modify. */
    /**
     * Strength of the modification.
     * - Zero means permanent, e.g. equipment.
     * - Non-zero means spell. Can only be removed by a wizard with a
     *   dispell level higher than this value.
     */
    unsigned char level;
};

typedef std::vector< AttributeModifier > AttributeModifiers;

/**
 * Type definition for a list of hits
 */
typedef std::vector<unsigned int> Hits;

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

        /**
         * Cleans obsolete attribute modifiers.
         */
        virtual void update();

        /**
         * Takes a damage structure, computes the real damage based on the
         * stats, deducts the result from the hitpoints and adds the result to
         * the HitsTaken list.
         */
        virtual int damage(Object *source, Damage const &damage);

        /**
         * Changes status and calls all the "died" listeners.
         */
        virtual void died();

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
        void performAttack(Damage const &, AttackZone const *attackZone);

        /**
         * Sets the current action.
         */
        void setAction(Action action);

        /**
         * Sets the current action.
         */
        Action getAction() const
        { return mAction; }

        /**
         * Gets the type of the attack the being is currently performing.
         */
        virtual int getAttackType() const
        { return 0; }

        /**
         * Moves the being toward its destination.
         */
        void move();

        /**
         * Sets an attribute.
         */
        void setAttribute(int n, int value)
        { mAttributes[n].base = value; }

        /**
         * Gets an attribute.
         */
        int getAttribute(int n) const
        { return mAttributes[n].base; }

        /**
         * Gets an attribute after applying modifiers.
         */
        int getModifiedAttribute(int) const;

        /**
         * Adds a modifier to one attribute.
         * @param duration If non-zero, creates a temporary modifier that
         *        expires after \p duration ticks.
         * @param lvl If non-zero, indicates that a temporary modifier can be
         *        dispelled prematuraly by a spell of given level.
         */
        void applyModifier(int attr, int value, int duration = 0, int lvl = 0);

        /**
         * Removes all the modifiers with a level low enough.
         */
        void dispellModifiers(int level);

        /**
         * Called when an attribute modifier is changed.
         */
        virtual void modifiedAttribute(int) {}

        /** Gets the name of the being. */
        std::string const &getName() const
        { return mName; }

        /** Sets the name of the being. */
        void setName(const std::string &name)
        { mName = name; }

        /**
         * Converts a direction to an angle. Used for combat hit checks.
         */
        static int directionToAngle(int direction);

    protected:
        static const int TICKS_PER_HP_REGENERATION = 100;
        Action mAction;
        std::vector< Attribute > mAttributes;

    private:
        Being(Being const &rhs);
        Being &operator=(Being const &rhs);

        std::string mName;
        Hits mHitsTaken; /**< List of punches taken since last update. */
        AttributeModifiers mModifiers; /**< Currently modified attributes. */
        int mHpRegenTimer; /**< Timer for hp regeneration. */
};

#endif // _TMWSERV_BEING_H_
