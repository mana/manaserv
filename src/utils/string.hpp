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

#ifndef UTILS_STRING_HPP
#define UTILS_STRING_HPP

#include <string>

namespace utils
{
    std::string toupper(std::string);
    bool isNumeric(const std::string &);
    int stringToInt(const std::string &);

    /**
     * Compares the two strings case-insensitively.
     *
     * @param a the first string in the comparison
     * @param b the second string in the comparison
     * @return 0 if the strings are equal, positive if the first is greater,
     *           negative if the second is greater
     */
    int compareStrI(const std::string &a, const std::string &b);
}

#endif // UTILS_STRING_HPP
