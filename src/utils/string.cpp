/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
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

#include "utils/string.hpp"

#include <cctype>
#include <algorithm>
#include <sstream>

std::string utils::toupper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), (int(*)(int)) std::toupper);
    return s;
}

bool utils::isNumeric(const std::string &s)
{
    for (unsigned int i = 0; i < s.size(); ++i)
    {
        if (!isdigit(s[i]))
        {
            return false;
        }
    }

    return true;
}

int utils::stringToInt(const std::string &s)
{
    int value;
    std::stringstream str(s);

    // put the string into the int
    str >> value;

    return value;
}
