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

#include <cassert>

#include "game-server/mapmanager.hpp"

#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapreader.hpp"
#include "game-server/resourcemanager.hpp"
#include "utils/logger.h"
#include "utils/xml.hpp"

/**
 * List of all the game maps, be they present or not on this server.
 */
static MapManager::Maps maps;

const MapManager::Maps &MapManager::getMaps()
{
    return maps;
}

unsigned int MapManager::initialize(const std::string &mapReferenceFile)
{
    // Indicates the number of maps loaded successfully
    unsigned int loadedMaps = 0;

    int size;
    char *data = ResourceManager::loadFile(mapReferenceFile, size);

    if (!data) {
        LOG_ERROR("Map Manager: Could not find " << mapReferenceFile << "!");
        free(data);
        return loadedMaps;
    }

    xmlDocPtr doc = xmlParseMemory(data, size);
    free(data);

    if (!doc)
    {
        LOG_ERROR("Map Manager: Error while parsing map database ("
                  << mapReferenceFile << ")!");
        return loadedMaps;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node || !xmlStrEqual(node->name, BAD_CAST "maps"))
    {
        LOG_ERROR("Map Manager: " << mapReferenceFile
                  << " is not a valid database file!");
        xmlFreeDoc(doc);
        return loadedMaps;
    }

    LOG_INFO("Loading map reference...");
    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
        if (!xmlStrEqual(node->name, BAD_CAST "map")) {
            continue;
        }

        int id = XML::getProperty(node, "id", 0);
        std::string name = XML::getProperty(node, "name", std::string());
        std::string file = "maps/";
        bool mapFileExists = false;
        // Test id and map name
        if (id != 0 && !name.empty())
        {
            // Testing if the file is actually in the maps folder
            file += name + ".tmx";
            if (!ResourceManager::exists(file))
            {
                file += ".gz";
                if (ResourceManager::exists(file))
                {
                    mapFileExists = true;
                }
            }
            else
            {
                mapFileExists = true;
            }

            if (mapFileExists)
            {
                maps[id] = new MapComposite(id, name);
                loadedMaps++;
            }
        }
    }

    xmlFreeDoc(doc);
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
    Maps::iterator i = maps.find(mapId);
    return (i != maps.end()) ? i->second : NULL;
}

MapComposite *MapManager::getMap(const std::string &mapName)
{
   Maps::iterator i;
   for (i = maps.begin(); i != maps.end(); ++i)
   {
      if (i->second->getName() == mapName)
          return i->second;
   }
   return NULL;
}

bool MapManager::raiseActive(int mapId)
{
    Maps::iterator i = maps.find(mapId);
    assert(i != maps.end());
    MapComposite *composite = i->second;
    if (composite->isActive())
    {
        return true;
    }

    std::string file = "maps/" + composite->getName() + ".tmx";
    if (!ResourceManager::exists(file))
    {
        file += ".gz";
    }
    if (MapReader::readMap(file, composite))
    {
        LOG_INFO("Activated map \"" << file << "\" (id " << mapId << ")");
        return true;
    }
    else
    {
        LOG_WARN("Couldn't activate invalid map \"" << file << "\" (id " <<
                 mapId << ")");
        return false;
    }
}



