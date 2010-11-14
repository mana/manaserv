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

#include <map>
#include <vector>
#include <string>

#include "game-server/being.h"
#include "game-server/eventlistener.h"
#include "defines.h"

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

/**
 * Structure containing different attack types of a monster type
 */
struct MonsterAttack
{
    unsigned id;
    int priority;
    float damageFactor;
    int element;
    DMG_TY type;
    int preDelay;
    int aftDelay;
    int range;
    std::string scriptFunction;
};

typedef std::vector< MonsterAttack *> MonsterAttacks;

/**
 * Class describing the characteristics of a generic monster.
 */
class MonsterClass
{
    public:
        MonsterClass(int id):
            mId(id),
            mSpeed(1),
            mSize(16),
            mExp(-1),
            mAggressive(false),
            mTrackRange(1),
            mStrollRange(0),
            mMutation(0),
            mAttackDistance(0),
            mOptimalLevel(0),
            mScript("")
        {}

        /**
         * Returns monster type. This is the Id of the monster class.
         */
        int getId() const
        { return mId; }

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

        /** Sets range in tiles in which the monster moves around when idle. */
        void setStrollRange(unsigned range) { mStrollRange = range; }

        /**
         * Returns range in tiles in which the monster moves around when idle.
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

        /** Adds an attack to the monsters repertoire. */
        void addAttack(MonsterAttack *type) { mAttacks.push_back(type); }

        /** Returns all attacks of the monster. */
        const MonsterAttacks &getAttacks() const { return mAttacks; }

        /** sets the script file for the monster */
        void setScript(const std::string &filename) { mScript = filename; }

        /** Returns script filename */
        const std::string &getScript() const { return mScript; }

        /**
         * Randomly selects a monster drop
         * @returns A class of item to drop, or NULL if none was found.
         */
        ItemClass *getRandomDrop() const;

    private:
        unsigned short mId;
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
        MonsterAttacks mAttacks;
        std::string mScript;

        friend class MonsterManager;
};

/**
 * Structure holding possible positions relative to the target from which
 * the monster can attack
 */
struct AttackPosition
{
    AttackPosition(int posX, int posY, Direction dir):
        x(posX),
        y(posY),
        direction(dir)
    {}

    int x;
    int y;
    Direction direction;
};

/**
 * The class for a fightable monster with its own AI
 */
class Monster : public Being
{
    public:
        /** Time in game ticks until ownership of a monster can change. */
        static const int KILLSTEAL_PROTECTION_TIME = 100;

        /**
         * Constructor.
         */
        Monster(MonsterClass *);

        /**
         * Destructor.
         */
        ~Monster();

        /**
         * Returns monster specy.
         */
        MonsterClass *getSpecy() const
        { return mSpecy; }

        /**
         * Performs one step of controller logic.
         */
        void update();

        /**
         * Performs an attack, if needed.
         */
        void perform();

        /**
         * Loads a script file for this monster
         */
        void loadScript(const std::string &scriptName);

        /**
         *
         */
        virtual int getAttackType() const
        { return mCurrentAttack->id; }

        /**
         * Kills the being.
         */
        void died();

        /**
         * Alters hate for the monster
         */
        void changeAnger(Actor *target, int amount);

        /**
         * Calls the damage function in Being and updates the aggro list
         */
        virtual int damage(Actor *source, const Damage &damage);

        /**
         * Removes a being from the anger list.
         */
        void forgetTarget(Thing *being);

        /**
         * Returns the way the actor is blocked by other things on the map.
         */
        virtual unsigned char getWalkMask() const
        {
            // blocked walls, other monsters and players ( bin 1000 0011)
            return 0x83;
        }

    protected:
        /**
         * Returns the way the actor blocks pathfinding for other objects.
         */
        virtual Map::BlockType getBlockType() const
        { return Map::BLOCKTYPE_MONSTER; }

    private:
        static const int DECAY_TIME = 50;

        int calculatePositionPriority(Point position, int targetPriority);

        MonsterClass *mSpecy;

        /**
         * Stores individual script for the monster, when NULL the script
         * from mSpecy is used.
         */
        Script *mScript;

        /** Aggression towards other beings. */
        std::map<Being *, int> mAnger;

        /** Listener for updating the anger list. */
        EventListener mTargetListener;

        /**
         * Character who currently owns this monster (killsteal protection).
         */
        Character *mOwner;

        /** List of characters and their skills that attacked this monster. */
        std::map<Character *, std::set <size_t> > mExpReceivers;

        /**
         * List of characters who are entitled to receive exp (killsteal
         * protection).
         */
        std::set<Character *> mLegalExpReceivers;

        /** Attack the monster is currently performing. */
        MonsterAttack *mCurrentAttack;

        /**
         * Set positions relative to target from which the monster can attack.
         */
        std::list<AttackPosition> mAttackPositions;

        friend struct MonsterTargetEventDispatch;
};

#endif // MONSTER_H
