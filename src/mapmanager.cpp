/*
 *  The Mana World
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#include "mapmanager.h"

#include "mapreader.h"

#include "utils/logger.h"

namespace tmwserv
{


MapManager::~MapManager()
    throw()
{
}

void MapManager::loadMap(const std::string& mapFile)
{
    Map *map = MapReader::readMap("maps/" + mapFile);
    if (map == NULL)
    {
        LOG_ERROR("Error: Unable to load map file (" << mapFile << ")", 0);
    }
    else
    {
        LOG_INFO("Loaded map " << maps.size() << " (" << mapFile << ")", 0);
        maps[mapFile] = map;
    }
}

void MapManager::unloadMap(const std::string& mapFile)
{
    std::map<std::string, Map *>::iterator i;
    
    i = maps.find(mapFile);
    if (i != maps.end())
    {
        delete i->second;
        maps.erase(i);
        LOG_INFO("Unloaded map (" << mapFile << ")", 0);
    }
    else
    {
        LOG_WARN("Unable to unload map (" << mapFile << ")", 0);
    }
}

void MapManager::reloadMap(const std::string& mapFile)
{
    unloadMap(mapFile);
    loadMap(mapFile);
}

Map *MapManager::getMap(const std::string& mapFile)
{
    Map *result = NULL;
    std::map<std::string, Map *>::iterator i;

    i = maps.find(mapFile);
    if (i != maps.end())
    {
        result = i->second;
    }
    return result;
}


} // namespace tmwserv
