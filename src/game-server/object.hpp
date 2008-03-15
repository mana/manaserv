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

#include "point.h"
#include "game-server/thing.hpp"

/**
 * Flags that are raised as necessary. They trigger messages that are sent to
 * the clients.
 */
enum
{
    UPDATEFLAG_NEW_ON_MAP = 1,
    UPDATEFLAG_NEW_DESTINATION = 2,
    UPDATEFLAG_ATTACK = 4,
    UPDATEFLAG_ACTIONCHANGE = 8,
    UPDATEFLAG_LOOKSCHANGE = 16
};

/**
 * Generic client-visible object definition. Keeps track of position and what
 * to update clients about.
 */
class Object : public Thing
{
    public:
        /**
         * Constructor.
         */
        Object(int type)
          : Thing(type),
            mUpdateFlags(0),
            mPos(Point(0, 0))
        {}

        /**
         * Sets the coordinates.
         *
         * @param p the coordinates.
         */
        virtual void setPosition(const Point &p)
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
        char mUpdateFlags; /**< Changes in object status. */
        Point mPos;        /**< Coordinates. */
};

#endif // _TMWSERV_OBJECT_H_
