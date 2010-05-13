/*
 *  The Mana Server
 *  Copyright (C) 2010-2010  The Mana World Development Team
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

#ifndef PERMISSIONMANAGER_HPP
#define PERMISSIONMANAGER_HPP

#include <list>
#include <map>
#include <string>

class Character;

namespace PermissionManager
{
    enum Result
    {
        PMR_UNKNOWN,
        PMR_DENIED,
        PMR_ALLOWED
    };
    /**
     * Loads permission file.
     */
    void initialize(const std::string &);

    /**
     * Reloads permission file.
     */
    void reload();

    /**
     * Returns if the characters account has the given permission
     */
    Result checkPermission(const Character* character, std::string permission);
    Result checkPermission(unsigned char level, std::string permission);

    /**
     * Gets the permission class bitmask of a class alias
     */
    unsigned char getMaskFromAlias(const std::string & alias);

    /**
     * Gets a list of all permissions the character is having
     */
    std::list<std::string> getPermissionList(const Character* character);

    /**
     * Gets a list of all permissions classes the character is having
     */
    std::list<std::string> getClassList(const Character* character);

} // namespace PermissionManager

#endif // PERMISSIONMANAGER_HPP
