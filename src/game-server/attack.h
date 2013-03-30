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

#ifndef ATTACK_H
#define ATTACK_H

#include <cstddef>
#include <list>

#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include "common/defines.h"

#include "scripting/script.h"

#include "utils/xml.h"

#include "game-server/timeout.h"

/**
 * Structure that describes the severity and nature of an attack a being can
 * be hit by.
 */
struct Damage
{
    unsigned id;          /**< Id of the attack (needed for displaying animation clientside */
    unsigned skill;       /**< Skill used by source (needed for exp calculation) */
    unsigned short base;  /**< Base amount of damage. */
    unsigned short delta; /**< Additional damage when lucky. */
    unsigned short cth;   /**< Chance to hit. Opposes the evade attribute. */
    Element element;      /**< Elemental damage. */
    DamageType type;      /**< Damage type: Physical or magical? */
    bool trueStrike;      /**< Override dodge calculation */
    unsigned short range; /**< Maximum distance that this attack can be used from, in pixels */

    Damage():
        id(0),
        skill(0),
        base(0),
        delta(0),
        cth(0),
        element(ELEMENT_NEUTRAL),
        type(DAMAGE_OTHER),
        trueStrike(false),
        range(DEFAULT_TILE_LENGTH)
    {}
};

/**
 * Class that stores information about an auto-attack
 */

class Character;

struct AttackInfo
{
    public:
        AttackInfo(unsigned priority, const Damage &damage,
               unsigned short warmupTime, unsigned short cooldownTime,
               unsigned short reuseTime):
            mDamage(damage),
            mCooldownTime(cooldownTime),
            mWarmupTime(warmupTime),
            mReuseTime(reuseTime),
            mPriority(priority)
        {}

        unsigned short getWarmupTime() const
        { return mWarmupTime; }

        unsigned short getCooldownTime() const
        { return mCooldownTime; }

        unsigned short getReuseTime() const
        { return mReuseTime; }

        static AttackInfo *readAttackNode(xmlNodePtr node);

        Damage &getDamage()
        { return mDamage; }

        const Script::Ref &getScriptCallback() const
        { return mCallback; }

        void setCallback(Script *script)
        { script->assignCallback(mCallback); }

        unsigned getPriority() const
        { return mPriority; }

    private:
        Damage mDamage;

        /**
         * Value to reset the timer to (warmup + cooldown)
         */
        unsigned short mCooldownTime;

        /**
         * Pre-attack delay tick.
         * This MUST be smaller than or equal to the aspd!
         * So the attack triggers where timer == warmup, having gone through
         * aspd - warmup ticks.
         */
        unsigned short mWarmupTime;

        /**
         * The global cooldown that needs to be finished before the being can
         * use the next attack.
         */
        unsigned short mReuseTime;

        /**
         * Name of the script callback
         */
        Script::Ref mCallback;

        /**
         * Priority of the attack
         */
        unsigned mPriority;
};

class Attack
{
    public:
        Attack(AttackInfo *info):
            mInfo(info)
        {}

        AttackInfo *getAttackInfo()
        { return mInfo; }

        void markAsTriggered()
        { mReuseTimer.set(mInfo->getCooldownTime() + mInfo->getReuseTime()); }

        bool isUsuable() const
        { return mReuseTimer.expired(); }


    private:
        /**
         * Contains infos about cooldown/damage/etc
         */
        AttackInfo *mInfo;

        /**
         * Internal timer that checks time for reuse
         */
        Timeout mReuseTimer;
};

/**
 * Helper class for storing multiple auto-attacks.
 */
class Attacks : public sigc::trackable
{
    public:
        Attacks():
            mCurrentAttack(0)
        {}

        void add(AttackInfo *);
        void remove(AttackInfo *);
        void markAttackAsTriggered();
        Attack *getTriggerableAttack();
        void startAttack(Attack *attack);
        void getUsuableAttacks(std::vector<Attack *> *ret);

        /**
         * Tells the number of attacks available
         */
        unsigned getNumber()
        { return mAttacks.size(); }

        sigc::signal<void, Attack &> attack_added;
        sigc::signal<void, Attack &> attack_removed;

    private:
        std::vector<Attack> mAttacks;

        Attack *mCurrentAttack;

        /**
         * when greater than cooldown -> warming up
         * when equals cooldown       -> trigger attack
         * when smaller               -> cooling down
         */
        Timeout mAttackTimer;
};

#endif // ATTACK_H
