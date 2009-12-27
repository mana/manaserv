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

#include "game-server/skillmanager.hpp"

#include "utils/string.hpp"   // for the toupper function

#include <map>

typedef std::map< std::string, int > SkillMap;
static SkillMap skillMap;
static std::string skillReferenceFile;

void SkillManager::initialize(const std::string &file)
{
    skillReferenceFile = file;
    reload();
}

void SkillManager::reload()
{
    //...
    skillMap["UNARMED"] = 100;
    skillMap["KNIFE"] = 101;
}

int SkillManager::getIdFromString(std::string name)
{
    //check if already an integer, if yes just return it
    int val;
    val = atoi(name.c_str());
    if (val) return val;

    // convert to upper case for easier finding
    name = utils::toupper(name);
    // find it
    SkillMap::iterator i = skillMap.find(name);
    if (i == skillMap.end())
    {
        return 0;
    } else {
        return i->second;
    }
}
