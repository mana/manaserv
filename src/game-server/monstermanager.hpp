/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
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

#ifndef MONSTERMANAGER_HPP
#define MONSTERMANAGER_HPP

#include <string>

class MonsterClass;

namespace MonsterManager
{
    /**
     * Loads monster reference file.
     */
    void initialize(const std::string &);

    /**
     * Reloads monster reference file.
     */
    void reload();

    /**
     * Destroy monster classes.
     */
    void deinitialize();

    /**
     * Gets the MonsterClass having the given ID.
     */
    MonsterClass *getMonster(int id);
}

#endif // MONSTERMANAGER_HPP
