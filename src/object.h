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

#include <utility>
#include <vector>

#include "defines.h"
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
        Object(int type)
          : mType(type),
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
        void setXY(unsigned int x, unsigned int y)
        { mX = x; mY = y; }

        /**
         * Get the coordinates.
         *
         * @return the coordinates as a pair.
         */
        std::pair<unsigned int, unsigned int> getXY() const
        { return std::make_pair(mX, mY); }

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

    private:
        int mType; /**< Object type */
        unsigned int mX; /**< x coordinate */
        unsigned int mY; /**< y coordinate */
        unsigned int mMapId;  /**< id of the map being is on */

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
        MovingObject(int type)
          : Object(type)
        {}
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
