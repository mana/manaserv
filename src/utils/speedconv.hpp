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

#ifndef SPEEDCONV_HPP
#define SPEEDCONV_HPP

// Simple helper functions for converting between tiles per
// second and the internal speed representation

#include "defines.h"

namespace utils {
    /**
     * tpsToSpeed()
     * @param tps The speed value in tiles per second
     * @returns The speed value in the internal representation
     */
    double tpsToSpeed(double);
    /**
     * speedToTps()
     * @param speed The speed value in the internal representation
     * @returns The speed value in tiles per second
     */
    double speedToTps(double);
}

#endif // SPEEDCONV_HPP
