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

#ifndef _TMW_COLLISIONDETECTION_H
#define _TMW_COLLISIONDETECTION_H

class Point;

/**
 * This namespace collects all needed collision detection functions
 */
namespace Collision
{

    bool
    circleWithCirclesector(const Point &circlePos, int circleRadius,
                           const Point &secPos, int secRadius,
                           float secAngle, float secSize);
    /**
     * Checks if a disk and a circle-sector collide
     *
     * @param halfTopAngle
     *        The half-top-angle of the circle sector in degrees (0,359).
     * @param placeAngle
     *        The placement-angle of the circle sector in degrees (0,359).
     */
    bool
    diskWithCircleSector(const Point &diskCenter, int diskRadius,
                         const Point &sectorCenter, int sectorRadius,
                         int halfTopAngle, int placeAngle);

    /**
     * Checks if two circles intersect.
     */
    bool
    CircleWithCircle(const Point &center1, int radius1,
                     const Point &center2, int radius2);

}

#endif
