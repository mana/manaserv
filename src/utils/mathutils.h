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

#ifndef _TMWSERV_MATHUTILS_H_
#define _TMWSERV_MATHUTILS_H_

namespace utils
{
    namespace math
    {
        /**
         * A very fast function to calculate the approximate inverse square
         * root of a floating point value.
         */
        float fastInvSqrt(float x);

        /**
         * A helper function that uses the fastInvSqrt for getting the
         * normal squareroot.
         */
        float fastSqrt(float x);

        float cachedSin(int angle);

        float cachedCos(int angle);

        float cachedTan(int angle);

        void init();

    } // namespace math

} // namespace utils

#endif
