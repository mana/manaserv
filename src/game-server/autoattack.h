/*
 *  The Mana Server
 *  Copyright (C) 2010  The Mana Development Team
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

#ifndef AUTOATTACK_H
#define AUTOATTACK_H

#include <cstddef>
#include <list>

#include "common/defines.h"

/**
 * Structure that describes the severity and nature of an attack a being can
 * be hit by.
 */
struct Damage
{
    unsigned int skill;             /**< Skill used by source (needed for exp calculation) */
    unsigned short base;            /**< Base amount of damage. */
    unsigned short delta;           /**< Additional damage when lucky. */
    unsigned short cth;             /**< Chance to hit. Opposes the evade attribute. */
    unsigned char element;          /**< Elemental damage. */
    DamageType type;                /**< Damage type: Physical or magical? */
    bool trueStrike;                /**< Override dodge calculation */
    unsigned short range;           /**< Maximum distance that this attack can be used from, in pixels */

    Damage(unsigned int skill,
           unsigned short base,
           unsigned short delta,
           unsigned short cth,
           unsigned char element,
           DamageType type = DAMAGE_OTHER,
           unsigned short range = DEFAULT_TILE_LENGTH):
        skill(skill),
        base(base),
        delta(delta),
        cth(cth),
        element(element),
        type(type),
        trueStrike(false),
        range(range)
    {}
};

/**
 * Class that stores information about an auto-attack
 */

class AutoAttack
{
    public:
        AutoAttack(Damage &damage, unsigned int warmup, unsigned int cooldown):
            mDamage(damage),
            mTimer(0),
            mAspd(cooldown),
            mWarmup(warmup && warmup < cooldown ? warmup : cooldown >> 2)
        {}

        unsigned short getTimer() const { return mTimer; }
        bool tick() { return mTimer ? !--mTimer : false; }
        void reset() { mTimer = mAspd; }
        void softReset() { if (mTimer >= mWarmup) mTimer = mAspd; }
        void halt() { if (mTimer >= mWarmup) mTimer = 0; }
        bool isReady() const { return !(mTimer - mWarmup); }

        bool operator<(const AutoAttack &rhs) const
        { return mTimer < rhs.mTimer; }

        const Damage &getDamage() const { return mDamage; }

    private:
        Damage mDamage;

        /**
         * Internal timer that is modified each tick.
         *
         * When > warmup, the attack is warming up before a strike
         * When = warmup, the attack triggers, dealing damage to the target
         *  *if* the target is still in range.
         *  (The attack is canceled when the target moves out of range before
         *   the attack can hit, there should be a trigger for scripts here
         *   too)
         *  (Should the character automatically persue when the target is still
         *   visible in this case?)
         * When < warmup, the attack is cooling down after a strike. When in
         *  cooldown, the timer should not be soft-reset.
         * When 0, the attack is inactive (the character is doing something
         *  other than attacking and the attack is not in cooldown)
         */
        unsigned short mTimer;

        /**
         * Value to reset the timer to (warmup + cooldown)
         */
        unsigned short mAspd;

        /**
         * Pre-attack delay tick.
         * This MUST be smaller than or equal to the aspd!
         * So the attack triggers where timer == warmup, having gone through
         * aspd - warmup ticks.
         */
        unsigned short mWarmup;
};

/**
 * Helper class for storing multiple auto-attacks.
 */
class AutoAttacks
{
    public:
        /**
         * Whether the being has at least one auto attack that is ready.
         */
        void add(const AutoAttack &);
        void clear(); // Wipe the list completely (used in place of remove for now; FIXME)
        void start();
        void stop(); // If the character does some action other than attacking, reset all warmups (NOT cooldowns!)
        void tick(std::list<AutoAttack> *ret = 0);

        /**
         * Tells the number of attacks available
         */
        unsigned getAutoAttacksNumber()
        { return mAutoAttacks.size(); }

        /**
         * Tells whether the autoattacks are active.
         */
        bool areActive()
        { return mActive; }

    private:
        /**
         * Marks whether or not to keep auto-attacking. Cooldowns still need
         * to be processed when false.
         */
        bool mActive;
        std::list<AutoAttack> mAutoAttacks;
};

#endif // AUTOATTACK_H
