/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "game-server/collisiondetection.hpp"

#include <cmath>

#include "point.h"
#include "utils/mathutils.h"

#define D_TO_R 0.0174532925    // PI / 180
#define R_TO_D 57.2957795      // 180 / PI

// Tests to see if pos is between s1 degree and s2
#define test_degrees(pos,s1,s2) (pos > s1 && pos < s2) || (s1 > s2 && !(pos < s1 && pos > s2))


bool Collision::circleWithCirclesector(const Point &circlePos, int circleRadius,
                                       const Point &secPos, int secRadius,
                                       float secAngle, float secSize)
{
    float targetAngle;

    // Calculate distance
    int distX = circlePos.x - secPos.x;
    int distY = circlePos.y - secPos.y;
    float invDist = utils::math::fastInvSqrt(distX * distX + distY * distY);
    float dist = 1.0f / invDist;

    // If out of range we can't hit it
    if (dist > secRadius + circleRadius) {
        return false;
    }
    // If we are standing in it we hit it in any case
    if (dist < circleRadius) {
        return true;
    }

    // Calculate target angle
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

    // Calculate difference from segment angle
    float targetDiff = fabs(targetAngle - secAngle);
    if (targetDiff > M_PI)
    {
        targetDiff = fabs(targetDiff - M_PI * 2);
    }


    // Add hit circle
    secSize += asin(circleRadius * invDist) * 2;

    return (targetDiff < secSize * 0.5f);
}


bool Collision::diskWithCircleSector(const Point &diskCenter, int diskRadius,
                                     const Point &sectorCenter, int sectorRadius,
                                     int halfTopAngle, int placeAngle)
{
    float r1 = sectorRadius,
          r2 = diskRadius;

    float dx = sectorCenter.x - diskCenter.x,
          dy = sectorCenter.y - diskCenter.y;

    // d^2 = dx^2 + dy^2
    float d = ((dx * dx) + (dy * dy));

    // d^2 < r2^2
    if (d < r2 * r2)
        return true; // We are right on top of each other

    // d^2 > r1^2 + r2^2
    if (d > ((r1+r2) * (r1+r2)))
        return false; // The two circles do not touch

    float s1 = placeAngle - halfTopAngle,
          s2 = placeAngle + halfTopAngle;

    if (s1 >= 360)
        s1 -= 360;
    if (s1 < 0)
        s1 += 360;
    if (s2 >= 360)
        s2 -= 360;
    if (s2 < 0)
        s2 += 360;

    // Is the center point of circle 2 within circle 1?
    if (d < r1 * r1)
    {
        // Circle 2 degrees in respect to circle 1
        float c2dc1 = atan2(dy,dx) * R_TO_D;
        if (c2dc1 < 0)
            c2dc1 += 360;

        if (test_degrees(c2dc1, s1, s2))
            return true;

        // Since we are well within range, we might be
        // Too close, so we need to make sure two circles intersect
        d = sqrt(d);
        r1 = d;
    } else {
        d = sqrt(d);
    }

    float a = ((r1*r1) - (r2*r2) + (d*d)) / (2.0 * d);
    float axd = (a * dx) / d,
          ayd = (a * dy) / d,
          h = sqrt((r1*r1) - (a*a));

    float ix1 = axd + ((h * dx) / d),
          iy1 = ayd - ((h * dy) / d),
          ix2 = axd - ((h * dx) / d),
          iy2 = ayd + ((h * dy) / d);

    float idc1 = atan2(iy1,ix1) * R_TO_D;
    if (idc1 < 0)
        idc1 += 360;
    if (test_degrees(idc1, s1, s2))
        return true;

    idc1 = atan2(iy2,ix2) * R_TO_D;
    if (idc1 < 0)
        idc1 += 360;
    if (test_degrees(idc1, s1, s2))
        return true;

    // If we got to this point, it must be false
    return false;

}


/**
 * Collision of a Disk with a Circle-Sector
 *
 * For a detailled explanation of this function please see:
 * http://wiki.themanaworld.org/index.php/Collision_determination
 *
bool
Collision::diskWithCircleSector2(const Point &diskCenter, int diskRadius,
                                const Point &sectorCenter, int sectorRadius,
                                int halfTopAngle, int placeAngle)
{
    // Converting the radii to float
    float R = (float) sectorRadius;
    float Rp = (float) diskRadius;

    // Transform to the primary coordinate system
    float Px = diskCenter.x - sectorCenter.x;
    float Py = diskCenter.y - sectorCenter.y;

    // The values of the trigonomic functions (only have to be computed once)
    float sinAlpha = utils::math::cachedSin(halfTopAngle);
    float cosAlpha = utils::math::cachedCos(halfTopAngle);
    float sinBeta  = utils::math::cachedSin(placeAngle);
    float cosBeta  = utils::math::cachedCos(placeAngle);

    **
     * This bounding circle implementation can be used up and until a
     * half-top-angle of +/- 85 degrees. The bounding circle becomes
     * infinitly large at 90 degrees. Above about 60 degrees a bounding
     * half-circle with radius R becomes more efficient.
     * (The additional check for a region 1 collision can then be scrapped.)
     *

    // Calculating the coordinates of the disk's center in coordinate system 4
    float Px1 = Px * cosBeta + Py * sinBeta;
    float Py1 = Py * cosBeta - Px * sinBeta;

    // Check for an intersection with the bounding circle
    // (>) : touching is accepted
    if ((cosAlpha * Px1 * Px1 + cosAlpha * Py1 * Py1 - Px1 * R)
                                > (Rp * Rp * cosAlpha + Rp * R)) return false;

    // Check for a region 4 collision
    if ((Px*Px + Py*Py) <=  (Rp*Rp)) return true;

    // Calculating the coordinates of the disk's center in coordinate system 1
    Px1 = Px * (cosAlpha * cosBeta + sinAlpha * sinBeta)
        + Py * (cosAlpha * sinBeta - sinAlpha * cosBeta);
    Py1 = Py * (cosAlpha * cosBeta + sinAlpha * sinBeta)
        - Px * (cosAlpha * sinBeta - sinAlpha * cosBeta);

    // Check if P is in region 5 (using coordinate system 1)
    if ((Px1 >= 0.0f) && (Px1 <= R) && (Py1 <= 0.0f))
    {
        // Return true on intersection, false otherwise
        // (>=) : touching  is accepted
        return (Py1 >= -1.0f * Rp);
    }

    // Check if P is in region 3 (using coordinate system 1)
    if ((Px1 > R) && (Py1 <= 0.0f))
    {
        // Calculating the vector from point A to the disk center
        float distAx = Px - R * (cosAlpha * cosBeta + sinAlpha * sinBeta);
        float distAy = Py - R * (cosAlpha * sinBeta - sinAlpha * cosBeta);

        // Check for a region 3 collision
        return ((distAx * distAx + distAy * distAy) <= Rp * Rp);
    }

    // Discard, if P is in region 4 (was previously checked)
    if ((Px1 < 0.0f) && (Py1 <= 0.0f)) return false;

    float tan2Alpha = utils::math::cachedTan(2 * halfTopAngle);

    // Check if P is in region 1 (using coordinate system 1)
    if ((Px1 >= 0.0f) && (Py1 >= 0.0f) && (Py1 <= Px1 * tan2Alpha))
    {
        // Return true on intersection, false otherwise
        // (<=) : touching  is accepted
        return ((Px * Px + Py * Py) <= (R * R + Rp * Rp + 2.0f * R * Rp));
    }

    // Calculating the coordinates of the disk's center in coordinate system 3
    Px1 = Px * (cosAlpha * cosBeta - sinAlpha * sinBeta)
        + Py * (sinAlpha * cosBeta + cosAlpha * sinBeta);
    Py1 = Py * (cosAlpha * cosBeta - sinAlpha * sinBeta)
        - Px * (sinAlpha * cosBeta + cosAlpha * sinBeta);

    // Discard, if P is in region 4 (was previously checked)
    if ((Px1 < 0.0f) && (Py1 >= 0.0f)) return false;

    // Check if P is in region 6 (using coordinate system 3)
    if ((Px1 >= 0.0f) && (Px1 <= R) && (Py1 >= 0.0f))
    {
        // Return true on intersection, false otherwise
        // (<=) : touching  is accepted
        return (Py1 <= Rp);
    }

    // Check if P is in region 2 (using coordinate system 3)
    if ((Px1 > R) && (Py1 <= 0.0f))
    {
        // Calculating the vector from point B to the disk center
        float distBx = Px - R * (cosAlpha * cosBeta - sinAlpha * sinBeta);
        float distBy = Py - R * (cosAlpha * sinBeta + sinAlpha * cosBeta);

        // Check for a region 2 collision
        return ((distBx * distBx + distBy * distBy) <= (Rp * Rp));
    }

    // The intersection with the bounding circle is in region 4,
    // but the disk and circle sector don't intersect.
    return false;
}
*/

bool Collision::circleWithCircle(const Point &center1, int radius1,
                                 const Point &center2, int radius2)
{
    int distx = center1.x - center2.x;
    int disty = center1.y - center2.y;
    double dist = sqrt((distx * distx) + (disty * disty));
    return (dist < radius1 + radius2);
}
