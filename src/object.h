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

enum
{
    NEW_ON_MAP = 1,
    NEW_DESTINATION = 2
};

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
        Object(int type)
          : mType(type),
            mUpdateFlags(0)
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
         * Update the internal status.
         */
        virtual void
        update() = 0;

        /**
         * Gets the map the object is located on.
         *
         * @return ID of map.
         */
        int getMapId() const
        { return mMapId; }

        /**
         * Sets the map the object is located on.
         */
        void setMapId(int mapId)
        { mMapId = mapId; }

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
        char mType; /**< Object type */
        char mUpdateFlags; /**< changes in object status */
        unsigned short mMapId;  /**< id of the map being is on */
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
          : Object(type), mPublicID(id),
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
        { mDst = dst; raiseUpdateFlags(NEW_DESTINATION); }

        /**
         * Gets the old coordinates of the object.
         */
        Point getOldPosition() const
        { return mOld; }

        /**
         * Sets object speed.
         */
        void setSpeed(unsigned s)
        { mSpeed = s; }

        /**
         * Moves the object toward its destination.
         */
        void move();

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
        unsigned short mActionTime; /**< delay until next action */
};

/**
 * Type definition for a smart pointer to Object.
 */
typedef utils::CountedPtr<Object> ObjectPtr;

/**
 * Type definition for a smart pointer to MovingObject.
 */
typedef utils::CountedPtr<MovingObject> MovingObjectPtr;


/**
 * Type definition for a list of Objects.
 */
typedef std::vector<ObjectPtr> Objects;

#endif // _TMWSERV_OBJECT_H_
