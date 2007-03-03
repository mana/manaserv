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
 *  $Id: $
 */

#include "mathutils.h"

#include <cmath>
#include <float.h>

#define MATH_UTILS_MAX_ANGLE 360

float sinList[MATH_UTILS_MAX_ANGLE];
float cosList[MATH_UTILS_MAX_ANGLE];
float tanList[MATH_UTILS_MAX_ANGLE];

/*
 * A very fast function to calculate the approximate inverse square root of a
 * floating point value. For an explanation of the inverse squareroot function
 * read:
 * http://www.math.purdue.edu/~clomont/Math/Papers/2003/InvSqrt.pdf
 *
 * Unfortunately the original creator of this function seems to be unknown.
 *
 * TODO: - Make this function run on 64-bit architectures
 *       - Find out and fix why this function fails when compiled with the -O2
 *         optimization flag on
 *         gcc version 4.0.2 20050808 (prerelease) (Ubuntu 4.0.1-4ubuntu9).
 */
float utils::math::fastInvSqrt(float x)
{
    float xhalf = 0.5f * x;
    int i = *(int*) &x;
    i = 0x5f375a86 - (i >> 1);
    x = *(float*) &i;
    x = x * (1.5f-xhalf * x * x);
    return x;
}

float utils::math::fastSqrt(float x)
{
    return 1.0f / utils::math::fastInvSqrt(x);
}

void utils::math::init()
{
    // Constant for calculating an angle in radians out of an angle in degrees
    const float radianAngleRatio = M_PI_2 / 90.0f; // pi/2 / 90[deg]

    for (int i = 0; i < MATH_UTILS_MAX_ANGLE; i++)
    {
        sinList[i] = sin(radianAngleRatio * (float) i);
        cosList[i] = cos(radianAngleRatio * (float) i);

        if (i == 90)
        {
            tanList[i] = FLT_MAX; // approximately infinity
            continue;
        }
        if (i == 270)
        {
            tanList[i] = -FLT_MAX; // approximately infinity
            continue;
        }
        tanList[i] = tan(radianAngleRatio * (float) i);
    }
}

float utils::math::cachedSin(int angle)
{
    return sinList[angle];
}

float utils::math::cachedCos(int angle)
{
    return cosList[angle];
}

float utils::math::cachedTan(int angle)
{
    return tanList[angle];
}
