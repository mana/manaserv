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
        MonsterClass(int id): mID(id) {}

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
         * Randomly selects a monster drop (may return NULL).
         * TODO: pass some luck modifier as an argument.
         */
        ItemClass *getRandomDrop() const;

    private:
        unsigned short mID; /**< ID of the monster class. */
        MonsterDrops mDrops; /**< Items the monster drops when dying. */
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
        int mAttackTime;                       /**< Delay until monster can attack */
        // TODO: the following vars should all be the same for all monsters of
        // the same type. So they should be put into some central data structure
        // to save memory.
        int mAttackPreDelay;        /**< time between decision to make an attack and performing the attack */
        int mAttackAftDelay;        /**< time it takes to perform an attack */
        int mAttackRange;           /**< range of the monsters attacks in pixel */
        int mAttackAngle;           /**< angle of the monsters attacks in degree */
        bool mAgressive;            /**< Does the monster attack without being provoked? */
        unsigned mAgressionRange;   /**< Distance the monster tracks enemies in */
        std::list<AttackPosition> mAttackPositions; /**< set positions relative to target from which the monster can attack */

        friend struct MonsterTargetEventDispatch;
};

#endif // _TMWSERV_MONSTER_H_
