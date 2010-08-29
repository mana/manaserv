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

#ifndef ATTRIBUTE_HPP
#define ATTRIBUTE_HPP

#include "defines.h"
#include <vector>
#include <list>

class AttributeModifierState
{
    public:
        AttributeModifierState(unsigned short duration, double value,
                               unsigned int id)
                : mDuration(duration), mValue(value), mId(id) {}
        ~AttributeModifierState() {}
        bool tick() { return mDuration ? !--mDuration : false; }
    private:
        /** Number of ticks (0 means permanent, e.g. equipment). */
        unsigned short mDuration;
        const double mValue;   /**< Positive or negative amount. */
        /**
         * Special purpose variable used to identify this effect to
         * dispells or similar. Exact usage depends on the effect,
         * origin, etc.
         */
        const unsigned int mId;
        friend bool durationCompare(const AttributeModifierState*,
                                    const AttributeModifierState*);
        friend class AttributeModifiersEffect;
};

class AttributeModifiersEffect {
    public:
        AttributeModifiersEffect(AT_TY sType, AME_TY eType);
        ~AttributeModifiersEffect();
        /**
         * Recalculates the value for this level.
         * @returns True if the value changed, false if it did not change.
         * Note that this will not change values at a higher level, nor the
         *     overall modified value for this attribute.
         * If this returns true, the cached values for *all* modifiers of a
         *     higher level must be recalculated, as well as the final
         */
        bool add(unsigned short duration, double value,
                 double prevLayerValue, int level);

        /**
         * remove() - as with Attribute::remove().
         */

        bool remove(double value, unsigned int id, bool fullCheck);

        /**
         * Performs the necessary modifications to mMod when the states change.
         */

        void updateMod(double value = 0);

        /**
         * Performs the necessary modifications to mCacheVal when the states
         * change.
         */

        bool recalculateModifiedValue(double newPrevLayerValue);

        double getCachedModifiedValue() const { return mCacheVal; }

        bool tick();

        /**
         * clearMods() - removes all modifications present in this layer.
         * This only really makes sense when all other layers are being reset too.
         * @param baseValue the value to reset to - typically an Attribute's mBase
         */

        void clearMods(double baseValue);

    private:
        /** List of all modifications present at this level */
        std::list< AttributeModifierState * > mStates;
        /**
         * Stores the value that results from mStates. This takes into
         * account all previous layers.
         */
        double mCacheVal;
        /**
         * Stores the effective modifying value from mStates. This defaults to
         * 0 for additive modifiers and 1 for multiplicative modifiers.
         */
        double mMod;
        const AT_TY  mSType;
        const AME_TY mEType;
};

class Attribute
{
    public:
        Attribute() {throw;} // DEBUG; Find improper constructions

        Attribute(const std::vector<struct AttributeInfoType> &type);

        ~Attribute();

        void setBase(double base);
        double getBase() const { return mBase; }
        double getModifiedAttribute() const
        { return mMods.empty() ? mBase :
                                 (*mMods.rbegin())->getCachedModifiedValue(); }

        /**
         * add() and remove() are the standard functions used to add and
         * remove modifiers while keeping track of the modifier state.
         */

        /**
         * @param duration The amount of time before the modifier expires
         *        naturally.
         *        When set to 0, the effect does not expire.
         * @param value The value to be applied as the modifier.
         * @param layer The id of the layer with which this modifier is to be
         *        applied to.
         * @param id Used to identify this effect.
         * @return Whether the modified attribute value was changed.
         */

        bool add(unsigned short duration, double value, unsigned int layer, int id = 0);

        /**
         * @param value The value of the modifier to be removed.
         *           - When 0, id is used exclusively to identify modifiers.
         * @param layer The id of the layer which contains the modifier to be removed.
         * @param id Used to identify this effect.
         *           - When 0, only the first match will be removed.
         *           - When non-0, all modifiers matching this id and other
         *                 parameters will be removed.
         * @param fullcheck Whether to perform a check for all modifiers,
         *     or only those that are otherwise permanent (ie. duration of 0)
         * @returns Whether the modified attribute value was changed.
         */
        bool remove(double value, unsigned int layer, int id, bool fullcheck);

        /**
         * clearMods() removes *all* modifications present in this Attribute (!)
         */

        void clearMods();

        /**
         * tick() processes all timers associated with modifiers for this attribute.
         */

        bool tick();

    private:
        double mBase;
        std::vector< AttributeModifiersEffect * > mMods;
};

#endif // ATTRIBUTE_HPP
