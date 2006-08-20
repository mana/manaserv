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

#include "map.h"
#include "mapreader.h"
#include "storage.h"

#include "utils/logger.h"

MapManager::~MapManager()
    throw()
{
}

Map *MapManager::loadMap(const unsigned int mapId)
{
    Storage &store = Storage::instance("tmw");
    std::string mapFile = store.getMapNameFromId(mapId);
    Map *map = MapReader::readMap("maps/" + mapFile);
    if (map == NULL)
    {
        LOG_ERROR("Unable to load map \"" << mapFile << "\" (id " << mapId
                << ")", 0);
    }
    else
    {
        LOG_INFO("Loaded map \"" << mapFile << "\" (id " << mapId << ")", 0);
        maps[mapId] = map;
    }
    return map;
}

void MapManager::unloadMap(const unsigned int mapId)
{
    std::map<unsigned int, Map *>::iterator i;

    i = maps.find(mapId);
    if (i != maps.end())
    {
        delete i->second;
        maps.erase(i);
        LOG_INFO("Unloaded map (" << mapId << ")", 0);
    }
    else
    {
        LOG_WARN("Unable to unload map (" << mapId << ")", 0);
    }
}

void MapManager::reloadMap(const unsigned int mapId)
{
    unloadMap(mapId);
    loadMap(mapId);
}

Map *MapManager::getMap(const unsigned int mapId)
{
    Map *result = NULL;
    std::map<unsigned int, Map *>::iterator i;

    i = maps.find(mapId);
    if (i != maps.end())
    {
        result = i->second;
    }
    else
    {
        result = loadMap(mapId);
    }
    return result;
}

bool
MapManager::isLoaded(const unsigned int mapId) const
{
    return maps.find(mapId) != maps.end();
}
