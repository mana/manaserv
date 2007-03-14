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


#ifndef _TMWSERV_OBJECT_H_
#define _TMWSERV_OBJECT_H_

#include <vector>

#include "point.h"
#include "game-server/map.hpp"

// Object type enumeration
enum
{
    OBJECT_ITEM = 0, // A simple item
    OBJECT_ACTOR,    // An item that toggle map/quest actions (doors, switchs, ...) and can speak (map panels).
    OBJECT_NPC,      // Non-Playable-Character is an actor capable of movement and maybe actions
    OBJECT_MONSTER,  // A monster (moving actor with AI. Should be able to toggle map/quest actions, too)
    OBJECT_CHARACTER,// A normal being
    OBJECT_OTHER     // Server-only object
};

class MapComposite;

enum
{
    UPDATEFLAG_NEW_ON_MAP = 1,
    UPDATEFLAG_NEW_DESTINATION = 2,
    UPDATEFLAG_ATTACK = 4,
    UPDATEFLAG_ACTIONCHANGE = 8,
    UPDATEFLAG_REMOVE = 16
};

/**
 * Base class for in-game objects.
 */
class Thing
{
    public:
        /**
         * Constructor.
         */
        Thing(int type)
          : mType(type)
        {}

        /**
         * Empty virtual destructor.
         */
        virtual ~Thing() {}

        /**
         * Gets type.
         *
         * @return the type.
         */
        int getType() const
        { return mType; }

        /**
         * Returns whether this thing is visible on the map or not. (Object)
         */
        bool isVisible() const
        { return mType != OBJECT_OTHER; }

        /**
         * Returns whether this thing can move on the map or not. (MovingObject)
         */
        bool canMove() const
        { return mType == OBJECT_CHARACTER || mType == OBJECT_MONSTER ||
                 mType == OBJECT_NPC; }

        /**
         * Returns whether this thing can fight or not. (Being)
         */
        bool canFight() const
        { return mType == OBJECT_CHARACTER || mType == OBJECT_MONSTER; }

        /**
         * Updates the internal status.
         */
        virtual void
        update() = 0;

        /**
         * Gets the map this thing is located on.
         *
         * @return ID of map.
         */
        int getMapId() const
        { return mMapId; }

        /**
         * Sets the map this thing is located on.
         */
        void setMapId(int mapId)
        { mMapId = mapId; }

    private:
        unsigned short mMapId;  /**< id of the map being is on */
        char mType; /**< Object type */
};

/**
 * Generic client-visible object definition.
 */
class Object: public Thing
{
    public:
        /**
         * Constructor.
         */
        Object(int type)
          : Thing(type),
            mUpdateFlags(0)
        {}

        /**
         * Sets the coordinates.
         *
         * @param p the coordinates.
         */
        void setPosition(const Point &p)
        { mPos = p; }

        /**
         * Gets the coordinates.
         *
         * @return the coordinates.
         */
        Point const &getPosition() const
        { return mPos; }

        /**
         * Gets what changed in the object.
         */
        int getUpdateFlags() const
        { return mUpdateFlags; }

        /**
         * Sets some changes in the object.
         */
        void raiseUpdateFlags(int n)
        { mUpdateFlags |= n; }

        /**
         * Clears changes in the object.
         */
        void clearUpdateFlags()
        { mUpdateFlags = 0; }

    private:
        char mUpdateFlags; /**< changes in object status */
        Point mPos; /**< coordinates */
};

/**
 * Base class for in-game moving objects.
 */
class MovingObject: public Object
{
    public:
        /**
         * Proxy constructor.
         */
        MovingObject(int type, int id)
          : Object(type),
            mPublicID(id),
            mDirection(0),
            mActionTime(0)
        {}

        /**
         * Gets the destination coordinates of the object.
         */
        Point const &getDestination() const
        { return mDst; }

        /**
         * Sets the destination coordinates of the object.
         */
        void setDestination(Point dst)
        { mDst = dst; raiseUpdateFlags(UPDATEFLAG_NEW_DESTINATION); mPath.clear(); }

        /**
         * Gets the old coordinates of the object.
         */
        Point getOldPosition() const
        { return mOld; }

        /**
         * Sets object direction
         */
        void setDirection(int direction)
        { mDirection = direction; }

        /**
         * Gets object direction
         */
        unsigned char getDirection() const
        { return mDirection; }

        /**
         * Sets object speed.
         */
        void setSpeed(unsigned s)
        { mSpeed = s; }

        /**
         * Sets object bounding circle radius
         */
        void setSize(unsigned s)
        { mSize = s; }

        /**
         * Gets object bounding circle radius
         */
        unsigned getSize()
        { return mSize; }

        /**
         * Moves the object toward its destination.
         */
        virtual void move();

        /**
         * Get public ID.
         *
         * @return the public ID, 65535 if none yet.
         */
        int getPublicID() const
        { return mPublicID; }

        /**
         * Set public ID.
         * The object shall not have any public ID yet.
         */
        void setPublicID(int id)
        { mPublicID = id; }

    private:
        unsigned short mPublicID; /**< Object ID sent to clients (unique with respect to the map) */
        Point mDst; /**< target coordinates */
        Point mOld; /**< old coordinates */
        unsigned short mSpeed; /**< speed */
        std::list<PATH_NODE> mPath;

    protected:
        unsigned char mDirection;   /**< Facing direction */
        unsigned short mActionTime; /**< delay until next action */
        unsigned mSize; /**< radius of bounding circle */
};

#endif // _TMWSERV_OBJECT_H_
