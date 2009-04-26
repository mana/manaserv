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

#ifndef _TMWSERV_POINT_H_
#define _TMWSERV_POINT_H_

#include <algorithm>

/**
 * A point in positive space. Usually represents pixel coordinates on a map.
 */
class Point
{
    public:
        Point():
            x(0), y(0)
        {}

        Point(unsigned short X, unsigned short Y):
            x(X), y(Y)
        {}

        unsigned short x; /**< x coordinate */
        unsigned short y; /**< y coordinate */

        /**
         * Check whether the given point is within range of this point.
         */
        bool inRangeOf(const Point &p, int radius) const
        {
            return std::abs(x - p.x) <= radius &&
                   std::abs(y - p.y) <= radius;
        }

        bool operator== (const Point &other) const
        {
            return (x == other.x && y == other.y);
        }

        bool operator!= (const Point &other) const
        {
            return (x != other.x || y != other.y);
        }
};

/**
 * A rectangle in positive space. Usually represents a pixel-based zone on a
 * map.
 */
class Rectangle
{
    public:
        unsigned short x; /**< x coordinate */
        unsigned short y; /**< y coordinate */
        unsigned short w; /**< width */
        unsigned short h; /**< height */

        bool contains(const Point &p) const
        {
            return (unsigned short)(p.x - x) < w &&
                   (unsigned short)(p.y - y) < h;
        }
};

#endif // _TMWSERV_POINT_H_
