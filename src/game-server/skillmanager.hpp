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


#ifndef SKILLMANAGER_H
#define SKILLMANAGER_H

#include <string>

namespace SkillManager
{
    /**
     * Loads skill reference file.
     */
    void initialize(const std::string &);

    /**
     * Reloads skill reference file.
     */
    void reload();

    /**
     * Gets the skill ID of a skill string
     * (not case-sensitive to reduce wall-bashing)
     */
    int getIdFromString(std::string name);  // no, thorbjorn, I am not passing this as const reference. I need a local copy.
}



#endif // SKILLMANAGER_H
