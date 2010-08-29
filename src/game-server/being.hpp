/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BEING_H
#define BEING_H

#include <string>
#include <vector>
#include <list>
#include <map>
#include "limits.h"

#include "game-server/actor.hpp"
#include "game-server/attribute.hpp"
#include "game-server/autoattack.hpp"

class Being;
class MapComposite;
class StatusEffect;

typedef std::map< unsigned int, Attribute > AttributeMap;

/**
 * Beings and actors directions
 * Needs to match client
 */
enum Direction
{
    DIRECTION_UP = 1,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
};

enum TimerID
{
    T_M_STROLL, // time until monster strolls to new location
    T_M_KILLSTEAL_PROTECTED,  // killsteal protection time
    T_M_DECAY,  // time until dead monster is removed
    T_M_ATTACK_TIME,    // time until monster can attack again
    T_B_HP_REGEN,    // time until hp is regenerated again
    T_C_MUTE // time until the character can chat again
};

struct Status
{
    StatusEffect *status;
    unsigned time;  // Number of ticks
};

typedef std::map< int, Status > StatusEffects;

/**
 * Type definition for a list of hits
 */
typedef std::vector<unsigned int> Hits;

/**
 * Generic being (living actor). Keeps direction, destination and a few other
 * relevant properties. Used for characters & monsters (all animated objects).
 */
class Being : public Actor
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
         * Moves enum for beings and actors for others players attack types.
         * WARNING: Has to be in sync with the same enum in the Being class
         * of the client!
         */
        enum AttackType
        {
            HIT = 0x00,
            CRITICAL = 0x0a,
            MULTI = 0x08,
            REFLECT = 0x04,
            FLEE = 0x0b
        };

        /**
         * Proxy constructor.
         */
        Being(ThingType type);

        /**
         * Cleans obsolete attribute modifiers.
         */
        virtual void update();

        /**
         * Takes a damage structure, computes the real damage based on the
         * stats, deducts the result from the hitpoints and adds the result to
         * the HitsTaken list.
         */
        virtual int damage(Actor *source, const Damage &damage);

        /** Restores all hit points of the being */
        void heal();

        /** Restores a specific number of hit points of the being */
        void heal(int hp);

        /**
         * Changes status and calls all the "died" listeners.
         */
        virtual void died();

        /**
         * Performs actions scheduled by the being.
         */
        virtual void perform() {}

        /**
         * Gets the destination coordinates of the being.
         */
        const Point &getDestination() const
        { return mDst; }

        /**
         * Sets the destination coordinates of the being.
         */
        void setDestination(const Point &dst);

        /**
         * Sets the destination coordinates of the being to the current
         * position.
         */
        void clearDestination()
        { setDestination(getPosition()); }

        /**
         * Gets the old coordinates of the being.
         */
        const Point &getOldPosition() const
        { return mOld; }

        /**
         * Sets the facing direction of the being.
         */
        void setDirection(int direction)
        { mDirection = direction; raiseUpdateFlags(UPDATEFLAG_DIRCHANGE); }

        int getDirection() const
        { return mDirection; }

        /**
         * Gets beings speed.
         * The speed is given in tiles per second.
         */

        /**
         * Gets beings speed.
         * The speed is to be set in tiles per second
         * This function automatically transform it
         * into millsecond per tile.
         */

        /**
         * Gets the damage list.
         */
        const Hits &getHitsTaken() const
        { return mHitsTaken; }

        /**
         * Clears the damage list.
         */
        void clearHitsTaken()
        { mHitsTaken.clear(); }

        /**
         * Performs an attack.
         * Return Value: damage inflicted or -1 when illegal target
         */
        int performAttack(Being *target, const Damage &damage);
        int performAttack(Being *target, unsigned range, const Damage &damage);

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
        { return HIT; }

        /**
         * Moves the being toward its destination.
         */
        void move();

        /**
         * Returns the path to the being's current destination.
         */
        virtual Path findPath();

        /**
         * Sets an attribute.
         */
        void setAttribute(unsigned int id, double value, bool calc = true);

        /**
         * Gets an attribute.
         */
        double getAttribute(unsigned int id) const;

        /**
         * Gets an attribute after applying modifiers.
         */
        double getModifiedAttribute(unsigned int id) const;

        /**
         * No-op to satisfy shared structure.
         * @note The game server calculates this manually, so nothing happens
         *       here.
         */
        void setModAttribute(unsigned int id, double value);

        /**
         * Checks whether or not an attribute exists in this being.
         * @returns True if the attribute is present in the being, false otherwise.
         */

        bool checkAttributeExists(unsigned int id) const
        { return mAttributes.count(id); }

        /**
         * Adds a modifier to one attribute.
         * @param duration If non-zero, creates a temporary modifier that
         *        expires after \p duration ticks.
         * @param lvl If non-zero, indicates that a temporary modifier can be
         *        dispelled prematuraly by a spell of given level.
         */
        void applyModifier(unsigned int attr, double value, unsigned int layer,
                           unsigned int duration = 0, unsigned int id = 0);

        bool removeModifier(unsigned int attr, double value, unsigned int layer,
                            unsigned int id = 0, bool fullcheck = false);

        /**
         * Called when an attribute modifier is changed.
         */
        virtual void updateDerivedAttributes(unsigned int) {}

        /**
         * Sets a statuseffect on this being
         */
        void applyStatusEffect(int id, int time);

        /**
         * Removes the status effect
         */
        void removeStatusEffect(int id);

        /**
         * Returns true if the being has a status effect
         */
        bool hasStatusEffect(int id) const;

        /**
         * Returns the time of the status effect if in effect, or 0 if not
         */
        unsigned getStatusEffectTime(int id) const;

        /**
         * Changes the time of the status effect (if in effect)
         */
        void setStatusEffectTime(int id, int time);

        /** Gets the name of the being. */
        const std::string &getName() const
        { return mName; }

        /** Sets the name of the being. */
        void setName(const std::string &name)
        { mName = name; }

        /**
         * Converts a direction to an angle. Used for combat hit checks.
         */
        static int directionToAngle(int direction);

        /**
         * Get Target
         */
        Being *getTarget() const
        { return mTarget; }

        /**
         * Set Target
         */
        void setTarget(Being *target)
        { mTarget = target; }


    protected:
        static const int TICKS_PER_HP_REGENERATION = 100;
        Action mAction;
        AttributeMap mAttributes;
        AutoAttacks mAutoAttacks;
        StatusEffects mStatus;
        Being *mTarget;
        Point mOld;                 /**< Old coordinates. */
        Point mDst;                 /**< Target coordinates. */

        /** Sets timer unless already higher. */
        void setTimerSoft(TimerID id, int value);

        /**
         * Sets timer even when already higher (when in doubt this one is
         * faster)
         */
        void setTimerHard(TimerID id, int value);

        /** Returns number of ticks left on the timer */
        int getTimer(TimerID id) const;

        /** Returns whether timer exists and is > 0 */
        bool isTimerRunning(TimerID id) const;

        /** Returns whether the timer reached 0 in this tick */
        bool isTimerJustFinished(TimerID id) const;

    private:
        Being(const Being &rhs);
        Being &operator=(const Being &rhs);

        Path mPath;
        unsigned char mDirection;   /**< Facing direction. */

        std::string mName;
        Hits mHitsTaken; /**< List of punches taken since last update. */

        typedef std::map<TimerID, int> Timers;
        Timers mTimers;
};

#endif // BEING_H
