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

#ifndef _TMWSERV_MOVINGOBJECT_H_
#define _TMWSERV_MOVINGOBJECT_H_

#include "point.h"
#include "game-server/map.hpp"
#include "game-server/object.hpp"


/**
 * Base class for in-game moving objects. This class adds a sense of direction,
 * destination and size.
 */
class MovingObject : public Object
{
    public:
        /**
         * Proxy constructor.
         */
        MovingObject(int type, int id)
          : Object(type),
            mPublicID(id),
            mActionTime(0),
            mDirection(0)
        {}

        virtual ~MovingObject() {};

        /**
         * Updates the walkmap of the map the object is on before
         * calling Object::setPosition
         */
        virtual void setPosition(const Point &p);

        /**
         * Gets the destination coordinates of the object.
         */
        Point const &getDestination() const
        { return mDst; }

        /**
         * Sets the destination coordinates of the object.
         */
        void setDestination(const Point &dst);

        /**
         * Sets the destination coordinates of the object to the current position.
         */
        void clearDestination()
        { setDestination(getPosition()); }

        /**
         * Gets the old coordinates of the object.
         */
        Point getOldPosition() const
        { return mOld; }

        /**
         * Sets object direction.
         */
        void setDirection(int direction)
        { mDirection = direction; raiseUpdateFlags(UPDATEFLAG_DIRCHANGE); }

        /**
         * Gets object direction.
         */
        int getDirection() const
        { return mDirection; }

        /**
         * Gets object speed.
         */
        int getSpeed() const
        { return mSpeed; }

        /**
         * Sets object speed.
         */
        void setSpeed(int s)
        { mSpeed = s; }

        /**
         * Sets object bounding circle radius.
         */
        void setSize(int s)
        { mSize = s; }

        /**
         * Gets object bounding circle radius.
         */
        int getSize()
        { return mSize; }

        /**
         * Performs actions scheduled by the object.
         */
        virtual void perform() {}

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
         * Set public ID. The object shall not have any public ID yet.
         */
        void setPublicID(int id)
        { mPublicID = id; }

        /**
         * Gets the way the object blocks pathfinding for other objects
         */
        virtual unsigned char getWalkMask() const
        { return 0x00; } //can walk through everything

        /**
         * Sets the map this thing is located on.
         */
        virtual void setMap(MapComposite *map);

    private:
        /** Object ID sent to clients (unique with respect to the map). */
        unsigned short mPublicID;

        Point mDst;                 /**< Target coordinates. */
        Point mOld;                 /**< Old coordinates. */
        unsigned short mSpeed;      /**< Speed. */
        std::list<PATH_NODE> mPath;

    protected:
        unsigned short mActionTime; /**< Delay until next action. */
        unsigned char mDirection;   /**< Facing direction. */
        unsigned char mSize;        /**< Radius of bounding circle. */

        /**
         * Gets the way the object blocks pathfinding for other objects
         */
        virtual Map::BlockType getBlockType() const
        { return Map::BLOCKTYPE_NONE; }
};

#endif // _TMWSERV_MOVINGOBJECT_H_
