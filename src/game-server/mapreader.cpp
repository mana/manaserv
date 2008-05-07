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

#include <cstring>

#include "game-server/mapreader.hpp"

#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/monstermanager.hpp"
#include "game-server/resourcemanager.hpp"
#include "game-server/spawnarea.hpp"
#include "game-server/trigger.hpp"
#include "scripting/script.hpp"
#include "utils/base64.h"
#include "utils/logger.h"
#include "utils/trim.hpp"
#include "utils/xml.hpp"
#include "utils/zlib.hpp"

static std::vector< int > tilesetFirstGids;

static Map* readMap(xmlNodePtr node, std::string const &path, MapComposite *composite, std::vector<Thing *> &things);
static void readLayer(xmlNodePtr node, Map *map);
static void setTileWithGid(Map *map, int x, int y, int gid);

void MapReader::readMap(const std::string &filename, MapComposite *composite)
{
    int fileSize;
    char *buffer = ResourceManager::loadFile(filename, fileSize);

    if (buffer == NULL)
    {
        LOG_ERROR("Error: Map file not found (" << filename.c_str() << ")");
        return;
    }

    xmlDocPtr doc = NULL;

    int l = filename.length();
    if (l > 3 && filename.substr(l - 3) == ".gz")
    {
        // Inflate the gzipped map data.
        char *inflated;
        unsigned inflatedSize = 0;
        bool ret = inflateMemory(buffer, fileSize, inflated, inflatedSize);
        free(buffer);
        buffer = ret ? inflated : NULL;
        fileSize = inflatedSize;
    }

    if (buffer)
    {
        // Parse the XML document.
        doc = xmlParseMemory(buffer, fileSize);
        free(buffer);
    }

    if (!doc)
    {
        LOG_ERROR("Error while parsing map file '" << filename << "'!");
        return;
    }

    Map *map = NULL;
    xmlNodePtr node = xmlDocGetRootElement(doc);

    std::vector<Thing *> things;

    // Parse the inflated map data.
    if (node && xmlStrEqual(node->name, BAD_CAST "map"))
    {
        map = ::readMap(node, filename, composite, things);
    }
    else
    {
        LOG_ERROR("Error: Not a map file (" << filename << ")!");
    }

    xmlFreeDoc(doc);

    if (map != NULL)
    {
        composite->setMap(map);

        for (std::vector< Thing * >::const_iterator i = things.begin(),
             i_end = things.end(); i != i_end; ++i)
        {
            composite->insert(*i);
        }

        if (Script *s = composite->getScript())
        {
            s->setMap(composite);
            s->prepare("initialize");
            s->execute();
        }
    }
}

static Map *readMap(xmlNodePtr node, std::string const &path, MapComposite *composite, std::vector<Thing *> &things)
{
    // Take the filename off the path
    std::string pathDir = path.substr(0, path.rfind("/") + 1);

    int w = XML::getProperty(node, "width", 0);
    int h = XML::getProperty(node, "height", 0);
    // We only support tile width of 32 at the moment
    //int tilew = getProperty(node, "tilewidth", DEFAULT_TILE_WIDTH);
    //int tileh = getProperty(node, "tileheight", DEFAULT_TILE_HEIGHT);
    int layerNr = 0;
    Map* map = new Map(w, h);

    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
        /* // Properties are useless server-side.
        if (xmlStrEqual(node->name, BAD_CAST "property"))
        {
            // Example: <property name="name" value="value"/>

            xmlChar *name = xmlGetProp(node, BAD_CAST "name");
            xmlChar *value = xmlGetProp(node, BAD_CAST "value");

            if (name && value)
            {
                map->setProperty((const char*)name, (const char*)value);
            }

            if (name) xmlFree(name);
            if (value) xmlFree(value);
        }
        else
        */
        if (xmlStrEqual(node->name, BAD_CAST "tileset"))
        {
            if (xmlHasProp(node, BAD_CAST "source"))
            {
                LOG_WARN("Warning: External tilesets not supported yet.");
            }
            else
            {
                tilesetFirstGids.push_back(XML::getProperty(node, "firstgid", 0));
            }
        }
        else if (xmlStrEqual(node->name, BAD_CAST "layer"))
        {
            // Layer 3 is collision layer.
            if (layerNr++ == 3)
            {
                readLayer(node, map);
            }
        }
        else if (xmlStrEqual(node->name, BAD_CAST "objectgroup"))
        {
            //readObjectGroup(node, map);
            for_each_xml_child_node(objectNode, node)
            {
                if (!xmlStrEqual(objectNode->name, BAD_CAST "object"))
                {
                    continue;
                }

                std::string objName = XML::getProperty(objectNode, "name", "");
                std::string objType = XML::getProperty(objectNode, "type", "");
                int objX = XML::getProperty(objectNode, "x", 0);
                int objY = XML::getProperty(objectNode, "y", 0);
                int objW = XML::getProperty(objectNode, "width", 0);
                int objH = XML::getProperty(objectNode, "height", 0);
                Rectangle rect = { objX, objY, objW, objH };


                if (objType == "WARP")
                {
                    int destMapId = -1;
                    int destX = -1;
                    int destY = -1;

                    for_each_xml_child_node(propertiesNode, objectNode)
                    {
                        if (!xmlStrEqual(propertiesNode->name, BAD_CAST "properties"))
                        {
                            continue;
                        }

                        for_each_xml_child_node(propertyNode, propertiesNode)
                        {
                            if (xmlStrEqual(propertyNode->name, BAD_CAST "property"))
                            {
                                std::string value = XML::getProperty(propertyNode, "name", std::string());
                                if (value == "DEST_MAP")
                                {
                                    destMapId = atoi((const char *)propertyNode->xmlChildrenNode->content);
                                }
                                else if (value == "DEST_X")
                                {
                                    destX = atoi((const char *)propertyNode->xmlChildrenNode->content);
                                }
                                else if (value == "DEST_Y")
                                {
                                    destY = atoi((const char *)propertyNode->xmlChildrenNode->content);
                                }
                            }
                        }
                    }

                    if (destMapId != -1 && destX != -1 && destY != -1)
                    {
                        MapComposite *destMap = MapManager::getMap(destMapId);
                        if (destMap)
                        {
                            things.push_back(new TriggerArea(
                                composite, rect,
                                new WarpAction(destMap, destX, destY),
                                false));
                        }
                    }
                    else
                    {
                        LOG_WARN("Unrecognized warp format");
                    }
                }
                else if (objType == "SPAWN")
                {
                    int monsterId = -1;
                    int maxBeings = 10; // Default value
                    int spawnRate = 10; // Default value

                    for_each_xml_child_node(propertiesNode, objectNode)
                    {
                        if (!xmlStrEqual(propertiesNode->name, BAD_CAST "properties"))
                        {
                            continue;
                        }

                        for_each_xml_child_node(propertyNode, propertiesNode)
                        {
                            if (xmlStrEqual(propertyNode->name, BAD_CAST "property"))
                            {
                                if (XML::getProperty(propertyNode, "name", std::string()) == "MONSTER_ID")
                                {
                                    monsterId = atoi((const char *)propertyNode->xmlChildrenNode->content);
                                }
                                else if (XML::getProperty(propertyNode, "name", std::string()) == "MAX_BEINGS")
                                {
                                    maxBeings = atoi((const char *)propertyNode->xmlChildrenNode->content);
                                }
                                else if (XML::getProperty(propertyNode, "name", std::string()) == "SPAWN_RATE")
                                {
                                    spawnRate = atoi((const char *)propertyNode->xmlChildrenNode->content);
                                }
                            }
                        }
                    }

                    MonsterClass *monster = MonsterManager::getMonster(monsterId);
                    if (monster != NULL)
                    {
                        things.push_back(new SpawnArea(composite, monster, rect, maxBeings, spawnRate));
                    }
                    else
                    {
                        LOG_WARN("Couldn't find monster ID " << monsterId <<
                                " for spawn area");
                    }
                }
                else if (objType == "NPC")
                {
                    Script *s = composite->getScript();
                    if (!s)
                    {
                        // Create a Lua context.
                        s = Script::create("lua");
                        composite->setScript(s);
                    }

                    int npcId = -1;
                    char const *scriptText = NULL;

                    for_each_xml_child_node(propertiesNode, objectNode)
                    {
                        if (!xmlStrEqual(propertiesNode->name, BAD_CAST "properties"))
                        {
                            continue;
                        }

                        for_each_xml_child_node(propertyNode, propertiesNode)
                        {
                            if (xmlStrEqual(propertyNode->name, BAD_CAST "property"))
                            {
                                std::string value = XML::getProperty(propertyNode, "name", std::string());
                                if (value == "NPC_ID")
                                {
                                    npcId = atoi((const char *)propertyNode->xmlChildrenNode->content);
                                }
                                else if (value == "SCRIPT")
                                {
                                    scriptText = (const char *)propertyNode->xmlChildrenNode->content;
                                }
                            }
                        }
                    }

                    if (npcId != -1 && scriptText != NULL)
                    {
                        s->loadNPC(npcId, objX, objY, scriptText);
                    }
                    else
                    {
                        LOG_WARN("Unrecognized format for npc");
                    }
                }
                else if (objType == "SCRIPT")
                {
                    Script *s = composite->getScript();
                    if (!s)
                    {
                        // Create a Lua context.
                        s = Script::create("lua");
                        composite->setScript(s);
                    }

                    std::string scriptFilename;
                    char const *scriptText = NULL;

                    for_each_xml_child_node(propertiesNode, objectNode)
                    {
                        if (!xmlStrEqual(propertiesNode->name, BAD_CAST "properties"))
                        {
                            continue;
                        }

                        for_each_xml_child_node(propertyNode, propertiesNode)
                        {
                            if (xmlStrEqual(propertyNode->name, BAD_CAST "property"))
                            {
                                std::string value = XML::getProperty(propertyNode, "name", std::string());
                                if (value == "FILENAME")
                                {
                                    scriptFilename = (const char *)propertyNode->xmlChildrenNode->content;
                                    trim(scriptFilename);
                                }
                                else if (value == "TEXT")
                                {
                                    scriptText = (const char *)propertyNode->xmlChildrenNode->content;
                                }
                            }
                        }
                    }

                    if (!scriptFilename.empty())
                    {
                        s->loadFile(scriptFilename);
                    }
                    else if (scriptText != NULL)
                    {
                        s->load(scriptText);
                    }
                    else
                    {
                        LOG_WARN("Unrecognized format for script");
                    }
                }
            }
        }
    }

    // Clean up tilesets
    tilesetFirstGids.clear();

    return map;
}

static void readLayer(xmlNodePtr node, Map *map)
{
    node = node->xmlChildrenNode;
    int h = map->getHeight();
    int w = map->getWidth();
    int x = 0;
    int y = 0;

    // Layers are assumed to be map size, with (0,0) as origin.
    // Find its single "data" element.
    while (node)
    {
        if (xmlStrEqual(node->name, BAD_CAST "data")) break;
        node = node->next;
    }

    if (!node)
    {
        LOG_WARN("Layer without any 'data' element.");
        return;
    }

    if (XML::getProperty(node, "encoding", std::string()) == "base64")
    {
        // Read base64 encoded map file
        xmlNodePtr dataChild = node->xmlChildrenNode;
        if (!dataChild)
        {
            LOG_WARN("Corrupted layer.");
            return;
        }

        int len = strlen((char const *)dataChild->content) + 1;
        char *charData = new char[len + 1];
        char const *charStart = (char const *)dataChild->content;
        char *charIndex = charData;

        while (*charStart)
        {
            if (*charStart != ' ' && *charStart != '\t' && *charStart != '\n')
            {
                *charIndex = *charStart;
                ++charIndex;
            }
            ++charStart;
        }
        *charIndex = '\0';

        int binLen;
        unsigned char *binData =
            php_base64_decode((unsigned char *)charData, strlen(charData), &binLen);

        delete[] charData;

        if (!binData)
        {
            LOG_WARN("Failed to decode base64-encoded layer.");
            return;
        }

        if (XML::getProperty(node, "compression", std::string()) == "gzip")
        {
            // Inflate the gzipped layer data
            char *inflated;
            unsigned inflatedSize;
            bool res = inflateMemory((char *)binData, binLen, inflated, inflatedSize);
            free(binData);

            if (!res)
            {
                LOG_WARN("Failed to decompress gzipped layer");
                return;
            }

            binData = (unsigned char *)inflated;
            binLen = inflatedSize;
        }

        for (int i = 0; i < binLen - 3; i += 4)
        {
            int gid = binData[i] |
                      (binData[i + 1] << 8)  |
                      (binData[i + 2] << 16) |
                      (binData[i + 3] << 24);

            setTileWithGid(map, x, y, gid);

            if (++x == w)
            {
                x = 0;
                ++y;
            }
        }
        free(binData);
        return;
    }

    // Read plain XML map file
    node = node->xmlChildrenNode;

    while (node)
    {
        if (xmlStrEqual(node->name, BAD_CAST "tile") && y < h)
        {
            int gid = XML::getProperty(node, "gid", -1);
            setTileWithGid(map, x, y, gid);

            if (++x == w)
            {
                x = 0;
                ++y;
            }
        }

        node = node->next;
    }
}

static void setTileWithGid(Map *map, int x, int y, int gid)
{
    // Find the tileset with the highest firstGid below/eq to gid
    int set = gid;
    for (std::vector< int >::const_iterator i = tilesetFirstGids.begin(),
         i_end = tilesetFirstGids.end(); i != i_end; ++i)
    {
        if (gid < *i)
        {
            break;
        }
        set = *i;
    }

    if (gid!=set)
    {
        map->blockTile(x, y, Map::BLOCKTYPE_WALL);
    }

}
