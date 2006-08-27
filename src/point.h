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

#ifndef _TMWSERV_POINT_H_
#define _TMWSERV_POINT_H_

#include "defines.h"

/**
 * A point in positive space. Usually representing pixel coordinates on a map.
 */
class Point
{
    public:
        unsigned int x; /**< x coordinate */
        unsigned int y; /**< y coordinate */

        /**
         * Constructor.
         */
        Point(unsigned int x, unsigned int y)
          : x(x),
            y(y)
        {}

        /**
         * Check whether the given point is range of this point. This is
         * defined as lying within the distance of client awareness.
         */
        bool inRangeOf(const Point &p) const
        {
            return (abs(x - p.x) <= (int) AROUND_AREA &&
                    abs(y - p.y) <= (int) AROUND_AREA);
        }
};

#endif // _TMWSERV_POINT_H_
