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

#ifndef _TMWSERV_MONSTER_H_
#define _TMWSERV_MONSTER_H_

#include <map>
#include <vector>

#include "game-server/attackzone.hpp"
#include "game-server/being.hpp"
#include "game-server/eventlistener.hpp"

class ItemClass;

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
    int type;
    int preDelay;
    int aftDelay;
    AttackZone attackZone;
};

typedef std::vector< MonsterAttack *> MonsterAttacks;

/**
 * Class describing the characteristics of a generic monster.
 */
class MonsterClass
{
    public:
        MonsterClass(int id):
            mID(id),
            mAttributes(BASE_ATTR_NB, 0),
            mSpeed(1),
            mSize(16),
            mExp(-1),
            mAggressive(false),
            mTrackRange(1),
            mStrollRange(0),
            mMutation(0),
            mAttackDistance(0)
        {}

        /**
         * Returns monster type. This is the ID of the monster class.
         */
        int getType() const
        { return mID; }

        /**
         * Sets monster drops. These are the items the monster drops when it
         * dies.
         */
        void setDrops(MonsterDrops const &v)
        { mDrops = v; }

        /**
         * Sets a being base attribute.
         */
        void setAttribute(size_t attribute, int value)
        { mAttributes.at(attribute) = value; }

        /**
         * Returns a being base attribute.
         */
        int getAttribute(size_t attribute) const
        { return mAttributes.at(attribute); }

        /** Sets inverted movement speed. */
        void setSpeed(int speed) { mSpeed = speed; }

        /** Returns inverted movement speed. */
        int getSpeed() const { return mSpeed; }

        /** Sets collision circle radius. */
        void setSize(int size) { mSize = size; }

        /** Returns collision circle radius. */
        int getSize() const { return mSize; }

        /** Sets experience reward for killing the monster. */
        void setExp(int exp) { mExp = exp; }

        /** Returns experience reward for killing the monster. */
        int getExp() const { return mExp; }

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

        /**
         * Randomly selects a monster drop (may return NULL).
         */
        ItemClass *getRandomDrop() const;

    private:
        unsigned short mID;
        MonsterDrops mDrops;
        std::vector<int> mAttributes; /**< Base attributes of the monster. */
        int mSpeed;
        int mSize;
        int mExp;

        bool mAggressive;
        unsigned mTrackRange;
        unsigned mStrollRange;
        unsigned mMutation;
        unsigned mAttackDistance;
        MonsterAttacks mAttacks;
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
    {};

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
        MonsterClass *getSpecy()
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
         *
         */
        virtual int getAttackType() const
        { return mCurrentAttack->id; }

        /**
         * Kills the being.
         */
        void died();

        /**
         * Calls the damage function in Being and updates the aggro list
         */
        virtual int damage(Object *source, Damage const &damage);

        /**
         * Removes a being from the anger list.
         */
        void forgetTarget(Thing *being);

        /**
         * Returns the way the object is blocked by other things on the map
         */
        virtual unsigned char getWalkMask() const
        {
            // blocked walls, other monsters and players ( bin 1000 0011)
            return 0x83;
        }

    protected:
        /**
         * Returns the way the object blocks pathfinding for other objects
         */
        virtual Map::BlockType getBlockType() const
        { return Map::BLOCKTYPE_MONSTER; }

    private:
        int calculatePositionPriority(Point position, int targetPriority);

        MonsterClass *mSpecy;

        /** Count down till next random movement (temporary). */
        int mCountDown;

        /** Aggression towards other beings. */
        std::map<Being *, int> mAnger;

        /** Listener for updating the anger list. */
        EventListener mTargetListener;

        /**
         * Character who currently owns this monster (killsteal protection).
         */
        Character* mOwner;

        /**
         * Time until someone else can claim this monster (killsteal
         * protection).
         */
        int mOwnerTimer;

        /** List of characters and their skills that attacked this monster. */
        std::map<Character *, std::set <size_t> > mExpReceivers;

        /**
         * List of characters who are entitled to receive exp (killsteal
         * protection).
         */
        std::set<Character *> mLegalExpReceivers;

        /** Delay until monster can attack. */
        int mAttackTime;

        /** Attack the monster is currently performing. */
        MonsterAttack *mCurrentAttack;

        /**
         * Set positions relative to target from which the monster can attack.
         */
        std::list<AttackPosition> mAttackPositions;

        friend struct MonsterTargetEventDispatch;
};

#endif // _TMWSERV_MONSTER_H_
