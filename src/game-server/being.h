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

#include "game-server/actor.h"
#include "game-server/attribute.h"
#include "game-server/attack.h"
#include "game-server/timeout.h"

class Being;
class MapComposite;
class StatusEffect;

typedef std::map< unsigned, Attribute > AttributeMap;

struct Status
{
    StatusEffect *status;
    unsigned time;  // Number of ticks
};

typedef std::map< int, Status > StatusEffects;

/**
 * Type definition for a list of hits
 */
typedef std::vector<unsigned> Hits;

/**
 * Generic being (living actor). Keeps direction, destination and a few other
 * relevant properties. Used for characters & monsters (all animated objects).
 */
class Being : public Actor
{
    public:
        /**
         * Proxy constructor.
         */
        Being(EntityType type);

        /**
         * Update being state.
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
         * Process all available attacks
         */
        void processAttacks();

        /**
         * Adds an attack to the available attacks
         */
        void addAttack(AttackInfo *attack);

        /**
         * Removes an attack from the available attacks
         */
        void removeAttack(AttackInfo *attackInfo);

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
        void setDirection(BeingDirection direction)
        { mDirection = direction; raiseUpdateFlags(UPDATEFLAG_DIRCHANGE); }

        BeingDirection getDirection() const
        { return mDirection; }

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
        int performAttack(Being *target, const Damage &dmg);

        /**
         * Sets the current action.
         */
        void setAction(BeingAction action);

        /**
         * Sets the current action.
         */
        BeingAction getAction() const
        { return mAction; }

        /**
         * Gets the attack id the being is currently performing.
         * For being, this is defaulted to the first one (1).
         */
        virtual int getAttackId() const
        { return mCurrentAttack ?
                mCurrentAttack->getAttackInfo()->getDamage().id : 0;
        }

        /**
         * Moves the being toward its destination.
         */
        void move();

        /**
         * Returns the path to the being's current destination.
         */
        virtual Path findPath();

        /** Gets the gender of the being (male or female). */
        BeingGender getGender() const
        { return mGender; }

        /** Sets the gender of the being (male or female). */
        void setGender(BeingGender gender);

        /**
         * Sets an attribute.
         */
        void setAttribute(unsigned id, double value);

        /**
         * Gets an attribute.
         */
        double getAttribute(unsigned id) const;

        /**
         * Gets an attribute after applying modifiers.
         */
        double getModifiedAttribute(unsigned id) const;

        /**
         * No-op to satisfy shared structure.
         * @note The game server calculates this manually, so nothing happens
         *       here.
         */
        void setModAttribute(unsigned, double);

        /**
         * Checks whether or not an attribute exists in this being.
         * @returns True if the attribute is present in the being, false otherwise.
         */

        bool checkAttributeExists(unsigned id) const
        { return mAttributes.count(id); }

        /**
         * Adds a modifier to one attribute.
         * @param duration If non-zero, creates a temporary modifier that
         *        expires after \p duration ticks.
         * @param lvl If non-zero, indicates that a temporary modifier can be
         *        dispelled prematuraly by a spell of given level.
         */
        void applyModifier(unsigned attr, double value, unsigned layer,
                           unsigned duration = 0, unsigned id = 0);

        bool removeModifier(unsigned attr, double value, unsigned layer,
                            unsigned id = 0, bool fullcheck = false);

        /**
         * Called when an attribute modifier is changed.
         * Recalculate the base value of an attribute and update derived
         *     attributes if it has changed.
         * @returns Whether it was changed.
         */
        virtual bool recalculateBaseAttribute(unsigned);

        /**
         * Attribute has changed, recalculate base value of dependant
         *     attributes (and handle other actions for the modified
         *     attribute)
         */
        virtual void updateDerivedAttributes(unsigned);

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

        /**
         * Overridden in order to reset the old position upon insertion.
         */
        virtual void inserted();

    protected:
        /**
         * Performs an attack
         */
        virtual void processAttack(Attack &attack);

        /**
         * Update the being direction when moving so avoid directions desyncs
         * with other clients.
         */
        void updateDirection(const Point &currentPos,
                             const Point &destPos);

        static const int TICKS_PER_HP_REGENERATION = 100;

        BeingAction mAction;
        AttributeMap mAttributes;
        Attacks mAttacks;
        StatusEffects mStatus;
        Being *mTarget;
        Point mOld;                 /**< Old coordinates. */
        Point mDst;                 /**< Target coordinates. */
        BeingGender mGender;        /**< Gender of the being. */
        Attack *mCurrentAttack;     /**< Last used attack. */


    private:
        Being(const Being &rhs);
        Being &operator=(const Being &rhs);

        Path mPath;
        BeingDirection mDirection;   /**< Facing direction. */

        std::string mName;
        Hits mHitsTaken; /**< List of punches taken since last update. */

        /** Time until hp is regenerated again */
        Timeout mHealthRegenerationTimeout;
};

#endif // BEING_H
