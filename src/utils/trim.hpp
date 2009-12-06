/*
 *  The Mana Server
 *  Copyright (C) 2007  The Mana World Development Team
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

#ifndef UTILS_TRIM_HPP
#define UTILS_TRIM_HPP

#include <string>

/**
 * Trims spaces off the end and the beginning of the given string.
 *
 * @param str the string to trim spaces off
 */
inline void trim(std::string &str)
{
    std::string::size_type pos = str.find_last_not_of(" \n\t");
    if (pos != std::string::npos)
    {
        str.erase(pos + 1);
        pos = str.find_first_not_of(" \n\t");
        if (pos != std::string::npos)
        {
            str.erase(0, pos);
        }
    }
    else
    {
        // There is nothing else but whitespace in the string
        str.clear();
    }
}

#endif
