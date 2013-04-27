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

#ifndef MONSTER_H
#define MONSTER_H

#include "game-server/being.h"
#include "common/defines.h"
#include "scripting/script.h"
#include "utils/string.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include <sigc++/connection.h>

class CharacterComponent;
class ItemClass;
class Script;

/**
 * Structure containing an item class and its probability to be dropped
 * (unit: 1/10000).
 */
struct MonsterDrop
{
    ItemClass *item;
    int probability;
};

typedef std::vector< MonsterDrop > MonsterDrops;

typedef std::map<Element, double> Vulnerabilities;

/**
 * Class describing the characteristics of a generic monster.
 */
class MonsterClass
{
    public:
        MonsterClass(int id):
            mId(id),
            mName("unnamed"),
            mGender(GENDER_UNSPECIFIED),
            mSpeed(1),
            mSize(16),
            mExp(-1),
            mAggressive(false),
            mTrackRange(1),
            mStrollRange(0),
            mMutation(0),
            mAttackDistance(0),
            mOptimalLevel(0)
        {}

        /**
         * Returns monster type. This is the Id of the monster class.
         */
        int getId() const
        { return mId; }

        /**
         * Returns the name of the monster type
         */
        const std::string &getName() const
        { return mName; }

        /**
         * Sets the name of the monster type
         */
        void setName(const std::string &name)
        { mName = name; }

        void setGender(BeingGender gender)
        { mGender = gender; }

        BeingGender getGender() const
        { return mGender; }

        /**
         * Sets monster drops. These are the items the monster drops when it
         * dies.
         */
        void setDrops(const MonsterDrops &v)
        { mDrops = v; }

        /**
         * Sets a being base attribute.
         */
        void setAttribute(int attribute, double value)
        { mAttributes[attribute] = value; }

        /**
         * Returns a being base attribute.
         */
        double getAttribute(int attribute) const
        { return mAttributes.at(attribute); }

        /**
         * Returns whether the monster has got the attribute.
         */
        bool hasAttribute(int attribute) const
        { return (mAttributes.find(attribute) != mAttributes.end()); }

        const std::map<int, double> &getAttributes() const
        { return mAttributes; }

        /** Sets collision circle radius. */
        void setSize(int size) { mSize = size; }

        /** Returns collision circle radius. */
        int getSize() const { return mSize; }

        /** Sets experience reward for killing the monster. */
        void setExp(int exp) { mExp = exp; }

        /** Returns experience reward for killing the monster. */
        int getExp() const { return mExp; }

        /** Gets maximum skill level after which exp reward is reduced */
        void setOptimalLevel(int level) { mOptimalLevel = level; }

        /** Sets maximum skill level after which exp reward is reduced. */
        int getOptimalLevel() const { return mOptimalLevel; }

        /** Sets if the monster attacks without being attacked first. */
        void setAggressive(bool aggressive) { mAggressive = aggressive; }

        /** Returns if the monster attacks without being attacked first. */
        bool isAggressive() const { return mAggressive; }

        /** Sets range in tiles in which the monster searches for enemies. */
        void setTrackRange(unsigned range){ mTrackRange = range; }

        /**
         * Returns range in tiles in which the monster searches for enemies.
         */
        unsigned getTrackRange() const { return mTrackRange; }

        /** Sets range in pixels in which the monster moves around when idle. */
        void setStrollRange(unsigned range) { mStrollRange = range; }

        /**
         * Returns range in pixels in which the monster moves around when idle.
         */
        unsigned getStrollRange() const { return mStrollRange; }

        /** Sets mutation factor in percent. */
        void setMutation(unsigned factor) { mMutation = factor; }

        /** Returns mutation factor in percent. */
        unsigned getMutation() const { return mMutation; }

        /** Sets preferred combat distance in pixels. */
        void setAttackDistance(unsigned distance)
        { mAttackDistance = distance; }

        /** Returns preferred combat distance in pixels. */
        unsigned getAttackDistance() const { return mAttackDistance; }

        void setVulnerability(Element element, double factor)
        { mVulnerabilities[element] = factor; }

        double getVulnerability(Element element) const;

        void setUpdateCallback(Script *script)
        { script->assignCallback(mUpdateCallback); }

        void setDamageCallback(Script *script)
        { script->assignCallback(mDamageCallback); }

        Script::Ref getUpdateCallback() const
        { return mUpdateCallback; }

        Script::Ref getDamageCallback() const
        { return mDamageCallback; }

    private:
        unsigned short mId;
        std::string mName;
        BeingGender mGender;

        MonsterDrops mDrops;
        std::map<int, double> mAttributes; /**< Base attributes of the monster. */
        float mSpeed; /**< The monster class speed in tiles per second */
        int mSize;
        int mExp;

        bool mAggressive;
        int mTrackRange;
        int mStrollRange;
        int mMutation;
        int mAttackDistance;
        int mOptimalLevel;
        Vulnerabilities mVulnerabilities;

        /**
         * A reference to the script function that is called each update.
         */
        Script::Ref mUpdateCallback;

        /**
         * A reference to the script that is called when a mob takes damage.
         */
        Script::Ref mDamageCallback;

        friend class MonsterManager;
        friend class MonsterComponent;
};

/**
 * The component for a fightable monster with its own AI
 */
class MonsterComponent : public Component
{
    public:
        static const ComponentType type = CT_Monster;

        /** Time in game ticks until ownership of a monster can change. */
        static const int KILLSTEAL_PROTECTION_TIME = 100;

        MonsterComponent(Entity &entity, MonsterClass *);
        ~MonsterComponent();

        /**
         * Returns monster specy.
         */
        MonsterClass *getSpecy() const
        { return mSpecy; }

        /**
         * Performs one step of controller logic.
         */
        void update(Entity &entity);

        /**
         * Signal handler
         */
        void monsterDied(Entity *monster);

        /**
         * Alters hate for the monster
         */
        void changeAnger(Entity *target, int amount);

        std::map<Entity *, int> getAngerList() const;

        /**
         * Removes a being from the anger list.
         */
        void forgetTarget(Entity *entity);

    private:
        static const int DECAY_TIME = 50;

        int calculatePositionPriority(Entity &entity,
                                      Point position,
                                      int targetPriority);

        MonsterClass *mSpecy;

        /** Aggression towards other beings. */
        struct AggressionInfo {
            AggressionInfo()
                : anger(0)
            {}

            int anger;
            sigc::connection removedConnection;
            sigc::connection diedConnection;
        };

        /**
         * Character who currently owns this monster (killsteal protection).
         */
        Entity *mOwner;

        /**
         * List of characters who are entitled to receive exp (killsteal
         * protection).
         */
        std::set<Entity *> mLegalExpReceivers;

        /** Time until monster strolls to new location */
        Timeout mStrollTimeout;
        /** Kill steal protection time */
        Timeout mKillStealProtectedTimeout;
        /** Time until dead monster is removed */
        Timeout mDecayTimeout;
};

#endif // MONSTER_H
