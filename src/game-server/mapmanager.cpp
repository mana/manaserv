/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *  Copyright (C) 2010-2011  The Mana Development Team
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

#include "game-server/mapmanager.h"

#include "common/resourcemanager.h"
#include "game-server/map.h"
#include "game-server/mapcomposite.h"
#include "utils/logger.h"
#include "utils/xml.h"

#include <cassert>

/**
 * List of all the game maps, be they present or not on this server.
 */
static MapManager::Maps maps;

const MapManager::Maps &MapManager::getMaps()
{
    return maps;
}

int MapManager::initialize(const std::string &mapReferenceFile)
{
    // Indicates the number of maps loaded successfully
    int loadedMaps = 0;

    XML::Document doc(mapReferenceFile);
    xmlNodePtr rootNode = doc.rootNode();

    if (!rootNode || !xmlStrEqual(rootNode->name, BAD_CAST "maps"))
    {
        LOG_ERROR("Item Manager: Error while parsing map database ("
                  << mapReferenceFile << ")!");
        return loadedMaps;
    }

    LOG_INFO("Loading map reference: " << mapReferenceFile);
    for_each_xml_child_node(node, rootNode)
    {
        if (!xmlStrEqual(node->name, BAD_CAST "map"))
            continue;

        int id = XML::getProperty(node, "id", 0);
        std::string name = XML::getProperty(node, "name", std::string());

        // Test id and map name
        if (id > 0 && !name.empty())
        {
            // Testing if the file is actually in the maps folder
            std::string file = std::string("maps/") + name + ".tmx";
            bool mapFileExists = ResourceManager::exists(file);

            // Try to fall back on fully compressed map
            if (!mapFileExists)
            {
                file += ".gz";
                mapFileExists = ResourceManager::exists(file);
            }

            if (mapFileExists)
            {
                maps[id] = new MapComposite(id, name);
                if (!maps[id]->readMap())
                    LOG_FATAL("Failed to load map \"" << name << "\"!");

                ++loadedMaps;
            }
        }
        else
        {
            if (name.empty())
            {
                LOG_WARN("Invalid unnamed map Id: " << id << '.');
            }
            else
            {
                LOG_WARN("Invalid map Id: " << id << " for map: "
                         << name << '.');
            }
        }
    }

    if (loadedMaps > 0)
        LOG_INFO(loadedMaps << " valid map file references were loaded.");

    return loadedMaps;
}

void MapManager::deinitialize()
{
    for (Maps::iterator i = maps.begin(), i_end = maps.end(); i != i_end; ++i)
    {
        delete i->second;
    }
    maps.clear();
}

MapComposite *MapManager::getMap(int mapId)
{
    Maps::const_iterator i = maps.find(mapId);
    return (i != maps.end()) ? i->second : NULL;
}

MapComposite *MapManager::getMap(const std::string &mapName)
{
    for (Maps::const_iterator i = maps.begin(); i != maps.end(); ++i)
        if (i->second->getName() == mapName)
            return i->second;

    return NULL;
}

bool MapManager::activateMap(int mapId)
{
    Maps::iterator i = maps.find(mapId);
    assert(i != maps.end());
    MapComposite *composite = i->second;

    if (composite->isActive())
        return true;

    if (composite->activate())
    {
        LOG_INFO("Activated map \"" << composite->getName()
                 << "\" (id " << mapId << ")");
        return true;
    }
    else
    {
        LOG_WARN("Couldn't activate invalid map \"" << composite->getName()
                 << "\" (id " << mapId << ")");
        return false;
    }
}
