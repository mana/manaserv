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
 *  $Id: being.cpp 3010 2007-01-05 20:12:51Z gmelquio $
 */

#include "collisiondetection.hpp"

#include "point.h"

#include <cmath>
#include "utils/fastsqrt.h"

bool
Collision::circleWithCirclesector(  const Point& circlePos, int circleRadius,
                                    const Point& secPos, int secRadius, float secAngle, float secSize)
{
    float targetAngle;

    //calculate distance
    int distX = circlePos.x - secPos.x;
    int distY = circlePos.y - secPos.y;
    float invDist = fastInvSqrt(distX * distX + distY * distY);
    float dist = 1.0f / invDist;

    //if out of range we can't hit it
    if (dist > secRadius + circleRadius) {
        return false;
    }
    //if we are standing in it we hit it in any case
    if (dist < circleRadius) {
        return true;
    }

    //calculate target angle
    if (distX > 0)
    {
        targetAngle = asin(-distY * invDist);
    } else {
        if (distY < 0)
        {
            targetAngle = M_PI - asin(-distY * invDist);
        } else {
            targetAngle = -M_PI - asin(-distY * invDist);
        }

    }

    //calculate difference from segment angle
    float targetDiff = fabs(targetAngle - secAngle);
    if (targetDiff > M_PI)
    {
        targetDiff = fabs(targetDiff - M_PI * 2);
    }


    //Add hit circle
    secSize += asin(circleRadius * invDist) * 2;

    return (targetDiff < secSize * 0.5f);
}
