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

#include "defines.h"
#include "point.h"
#include "utils/countedptr.h"

/**
 * Generic in-game object definition.
 * Base class for in-game objects.
 */
class Object
{
    public:
        /**
         * Constructor.
         */
        Object(int type, int id)
          : mType(type),
            mID(id),
            mNew(true),
            mNeedUpdate(false)
        {}

        /**
         * Empty virtual destructor.
         */
        virtual ~Object() {}

        /**
         * Get type.
         *
         * @return the type.
         */
        unsigned int getType() const
        { return mType; }

        /**
         * Get object ID.
         *
         * @return the unique ID, a negative number if none yet.
         */
        int getID() const
        { return mID; }

        /**
         * Set object ID.
         * The account shall not have any ID yet.
         */
        void setID(int id);

        /**
         * Set the x coordinate.
         *
         * @param x the new x coordinate.
         */
        void setX(unsigned int x)
        { mX = x; }

        /**
         * Get the x coordinate.
         *
         * @return the x coordinate.
         */
        unsigned int getX() const
        { return mX; }

        /**
         * Set the y coordinate.
         *
         * @param y the new y coordinate.
         */
        void setY(unsigned int y)
        { mY = y; }

        /**
         * Get the y coordinate.
         *
         * @return the y coordinate.
         */
        unsigned int getY() const
        { return mY; }

        /**
         * Set the coordinates.
         *
         * @param x the x coordinate.
         * @param y the y coordinate.
         */
        void setXY(const Point &p)
        { mX = p.x; mY = p.y; }

        /**
         * Get the coordinates.
         *
         * @return the coordinates as a pair.
         */
        Point getXY() const
        { return Point(mX, mY); }

        /**
         * Update the internal status.
         */
        virtual void
        update() = 0;

        /**
         * Get map name where being is
         *
         * @return Name of map being is located.
         */
        unsigned int getMapId() const
        { return mMapId; }

        /**
         * Set map being is located
         */
        void setMapId(unsigned int mapId)
        { mMapId = mapId; }

        /**
         * Tells if the object just appeared.
         */
        bool isNew() const
        { return mNew; }

        /**
         * Sets the age of the object.
         */
        void setNew(bool n)
        { mNew = n; }

    private:
        int mType; /**< Object type */
        int mID; /** Object unique ID (wrt its type and its map at least) */
        unsigned int mX; /**< x coordinate */
        unsigned int mY; /**< y coordinate */
        unsigned int mMapId;  /**< id of the map being is on */
        bool mNew; /**< true if the object just appeared */

    protected:
        bool mNeedUpdate;  /**< update() must be invoked if true */
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
          : Object(type, id)
        {}

        /**
         * Gets the destination coordinates of the object.
         */
        Point getDestination() const
        { return Point(mDstX, mDstY); }

        /**
         * Sets the destination coordinates of the object.
         */
        void setDestination(unsigned x, unsigned y)
        { mDstX = x; mDstY = y; }

        /**
         * Gets the next coordinates of the object.
         */
        Point getNextPosition() const
        { return Point(mNewX, mNewY); }

        /**
         * Sets object speed.
         */
        void setSpeed(unsigned s)
        { mSpeed = s; }

        /**
         * Moves the object toward its destination.
         */
        void move();

    private:
        unsigned mDstX; /**< target x coordinate */
        unsigned mDstY; /**< target y coordinate */
        unsigned mNewX; /**< next x coordinate */
        unsigned mNewY; /**< next y coordinate */
        unsigned mSpeed; /**< speed */
};

/**
 * Type definition for a smart pointer to Object.
 */
typedef utils::CountedPtr<Object> ObjectPtr;


/**
 * Type definition for a list of Objects.
 */
typedef std::vector<ObjectPtr> Objects;

#endif // _TMWSERV_OBJECT_H_
