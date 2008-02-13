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

#ifndef _TMWSERV_MONSTER_H_
#define _TMWSERV_MONSTER_H_

#include <map>
#include <vector>

#include "game-server/being.hpp"
#include "game-server/eventlistener.hpp"

class ItemClass;
class MapComposite;

/**
 * Structure containing an item class and its probability to be dropped (unit: 1/10000).
 */
struct MonsterDrop
{
    ItemClass *item;
    int probability;
};

typedef std::vector< MonsterDrop > MonsterDrops;

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
            mMutation(0)
        {}

        /**
         * Gets monster type.
         */
        int getType() const
        { return mID; }

        /**
         * Sets monster drops.
         */
        void setDrops(MonsterDrops const &v)
        { mDrops = v; }

        /**
         * Sets a being attribute
         */
        void setAttribute(size_t attribute, int value)
        { mAttributes.at(attribute) = value ; }

        /**
         * Gets a being attribute
         */
        int getAttribute(size_t attribute) const
        { return mAttributes.at(attribute); }

        void setSpeed(int speed) { mSpeed = speed; } /**< sets inverted movement speed*/
        int getSpeed() const { return mSpeed; } /**< gets inverted movement speed*/

        void setSize(int size) { mSize = size; } /**< sets hit circle radius*/
        int getSize() const { return mSize; } /**< gets hit circle radius*/

        void setExp(int exp) { mExp = exp; } /**< sets experience reward*/
        int getExp() const { return mExp; } /**< gets experience reward*/

        void setAggressive(bool aggressive) { mAggressive = aggressive; } /**< sets if the monster attacks without being attacked first*/
        bool isAggressive() const { return mAggressive; } /**< gets if the monster attacks without being attacked first*/

        void setTrackRange(unsigned range){ mTrackRange = range; } /**< sets range in tiles in which the monster searches for enemies*/
        unsigned getTrackRange() const { return mTrackRange; } /**< gets range in tiles in which the monster searches for enemies*/

        void setStrollRange(unsigned range) { mStrollRange = range; } /**< sets range in tiles in which the monster moves around when idled*/
        unsigned getStrollRange() const { return mStrollRange; } /**< gets range in tiles in which the monster moves around when idled*/

        void setMutation(unsigned factor) { mMutation = factor; } /**< sets mutation factor in percent*/
        unsigned getMutation() const { return mMutation; } /**< gets mutation factor in percent*/

        /**
         * Randomly selects a monster drop (may return NULL).
         */
        ItemClass *getRandomDrop() const;

    private:
        unsigned short mID; /**< ID of the monster class. */
        MonsterDrops mDrops; /**< Items the monster drops when dying. */
        std::vector<int> mAttributes; /**< Base attributes of the monster*/
        int mSpeed; /** (inverted) Movement speed of the monster */
        int mSize; /** Collision circle radius of the monster */
        int mExp; /**< Exp reward for killing the monster */
        bool mAggressive;       /**< Does the monster attack without being provoked? */
        unsigned mTrackRange;   /**< Distance the monster tracks enemies in */
        unsigned mStrollRange;  /**< Distance the monster strolls around in when not fighting */
        unsigned mMutation;     /**< Mutation factor in percent*/
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

        static const int KILLSTEAL_PROTECTION_TIME = 100; /**< Time in game ticks until ownership of a monster can change */

        /**
         * Constructor.
         */
        Monster(MonsterClass *);

        /**
         * Destructor.
         */
        ~Monster();

        /**
         * Gets monster specy.
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

    private:
        int calculatePositionPriority(Point position, int targetPriority);

        MonsterClass *mSpecy; /**< Monster specy. */
        int mCountDown; /**< Count down till next random movement (temporary). */
        std::map<Being *, int> mAnger;   /**< Aggression towards other beings */
        EventListener mTargetListener; /**< Listener for updating the anger list. */

        Character* mOwner; /**< Character who currently owns this monster (killsteal protection) */
        int mOwnerTimer; /**< Time until someone else can claim this monster (killsteal protection) */
        std::map<Character *, std::set <size_t> > mExpReceivers; /**< List of characters and their skills that attacked this monster*/
        std::set<Character *> mLegalExpReceivers; /**< List of characters who are entitled to receive exp (killsteal protection)*/

        int mAttackTime;                       /**< Delay until monster can attack */
        // TODO: the following vars should all be the same for all monsters of
        // the same type. So they should be stored in mSpecy to save memory
        int mAttackPreDelay;        /**< time between decision to make an attack and performing the attack */
        int mAttackAftDelay;        /**< time it takes to perform an attack */
        int mAttackRange;           /**< range of the monsters attacks in pixel */
        int mAttackAngle;           /**< angle of the monsters attacks in degree */
        std::list<AttackPosition> mAttackPositions; /**< set positions relative to target from which the monster can attack */

        friend struct MonsterTargetEventDispatch;
};

#endif // _TMWSERV_MONSTER_H_
