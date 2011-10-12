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

#include "game-server/mapreader.h"

#include "game-server/map.h"
#include "game-server/mapcomposite.h"
#include "game-server/mapmanager.h"
#include "game-server/monstermanager.h"
#include "game-server/spawnarea.h"
#include "game-server/trigger.h"
#include "scripting/script.h"
#include "utils/base64.h"
#include "utils/logger.h"
#include "utils/xml.h"
#include "utils/zlib.h"
#include "utils/string.h"

#include <cstring>

static std::vector< int > tilesetFirstGids;

bool MapReader::readMap(const std::string &filename, MapComposite *composite)
{
    XML::Document doc(filename);
    xmlNodePtr rootNode = doc.rootNode();

    // Parse the inflated map data.
    if (!rootNode || !xmlStrEqual(rootNode->name, BAD_CAST "map"))
    {
        LOG_ERROR("Error: Not a map file (" << filename << ")!");
        return false;
    }

    std::vector<Thing *> things;
    Map *map = readMap(rootNode, filename, composite, things);

    if (map)
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
    return true;
}

Map* MapReader::readMap(xmlNodePtr node, const std::string &path,
                        MapComposite *composite, std::vector<Thing *> &things)
{
    // Take the filename off the path
    std::string pathDir = path.substr(0, path.rfind("/") + 1);
    int w = XML::getProperty(node, "width", 0);
    int h = XML::getProperty(node, "height", 0);
    int tilew = XML::getProperty(node, "tilewidth", DEFAULT_TILE_LENGTH);
    int tileh = XML::getProperty(node, "tileheight", DEFAULT_TILE_LENGTH);
    Map* map = new Map(w, h, tilew, tileh);

    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
        if (xmlStrEqual(node->name, BAD_CAST "tileset"))
        {
            if (xmlHasProp(node, BAD_CAST "source"))
            {
                LOG_WARN("Warning: External tilesets not supported yet.");
            }
            else
            {
                ::tilesetFirstGids.push_back(XML::getProperty(node, "firstgid",
                                                              0));
            }
        }
        else if (xmlStrEqual(node->name, BAD_CAST "properties"))
        {
            for_each_xml_child_node(propNode, node)
            {
                if (xmlStrEqual(propNode->name, BAD_CAST "property"))
                {
                    std::string key = XML::getProperty(propNode, "name",
                                                       std::string());
                    std::string val = XML::getProperty(propNode, "value",
                                                       std::string());
                    LOG_DEBUG("  " << key << ": " << val);
                    map->setProperty(key, val);
                }
            }
        }
        else if (xmlStrEqual(node->name, BAD_CAST "layer"))
        {
            if (utils::compareStrI(XML::getProperty(node, "name", "unnamed"),
                                   "collision") == 0)
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

                std::string objName = XML::getProperty(objectNode, "name",
                                                       std::string());
                std::string objType = XML::getProperty(objectNode, "type",
                                                       std::string());
                objType = utils::toUpper(objType);
                int objX = XML::getProperty(objectNode, "x", 0);
                int objY = XML::getProperty(objectNode, "y", 0);
                int objW = XML::getProperty(objectNode, "width", 0);
                int objH = XML::getProperty(objectNode, "height", 0);
                Rectangle rect = { objX, objY, objW, objH };


                if (utils::compareStrI(objType, "WARP") == 0)
                {
                    std::string destMapName = std::string();
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
                            if (xmlStrEqual(propertyNode->name,
                                            BAD_CAST "property"))
                            {
                                std::string value = XML::getProperty(
                                           propertyNode, "name", std::string());
                                value = utils::toUpper(value);
                                if (utils::compareStrI(value, "DEST_MAP") == 0)
                                {
                                    destMapName = getObjectProperty(propertyNode,
                                                                 std::string());
                                }
                                else if (utils::compareStrI(value, "DEST_X") == 0)
                                {
                                    destX = getObjectProperty(propertyNode, -1);
                                }
                                else if (utils::compareStrI(value, "DEST_Y") == 0)
                                {
                                    destY = getObjectProperty(propertyNode, -1);
                                }
                            }
                        }
                    }

                    if (!destMapName.empty() && destX != -1 && destY != -1)
                    {
                        MapComposite *destMap = MapManager::getMap(destMapName);
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
                else if (utils::compareStrI(objType, "SPAWN") == 0)
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
                                std::string value = XML::getProperty(propertyNode, "name", std::string());
                                value = utils::toUpper(value);
                                if (utils::compareStrI(value, "MONSTER_ID") == 0)
                                {
                                    monsterId = getObjectProperty(propertyNode, monsterId);
                                }
                                else if (utils::compareStrI(value, "MAX_BEINGS") == 0)
                                {
                                    maxBeings = getObjectProperty(propertyNode, maxBeings);
                                }
                                else if (utils::compareStrI(value, "SPAWN_RATE") == 0)
                                {
                                    spawnRate = getObjectProperty(propertyNode, spawnRate);
                                }
                            }
                        }
                    }

                    MonsterClass *monster = monsterManager->getMonster(monsterId);
                    if (monster)
                    {
                        things.push_back(new SpawnArea(composite, monster, rect, maxBeings, spawnRate));
                    }
                    else
                    {
                        LOG_WARN("Couldn't find monster ID " << monsterId <<
                                " for spawn area");
                    }
                }
                else if (utils::compareStrI(objType, "NPC") == 0)
                {
                    Script *s = composite->getScript();
                    if (!s)
                    {
                        // Create a Lua context.
                        //std::string engineName= Script::determinateEngineOverFilename(filename.str());
                        std::string engineName="lua"; //TODO Determinate Scripttype
                        s = Script::create(engineName);
                        composite->setScript(s);
                    }

                    int npcId = -1;
                    std::string scriptText;

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
                                value = utils::toUpper(value);
                                if (utils::compareStrI(value, "NPC_ID") == 0)
                                {
                                    npcId = getObjectProperty(propertyNode, npcId);
                                }
                                else if (utils::compareStrI(value, "SCRIPT") == 0)
                                {
                                    scriptText = getObjectProperty(propertyNode, std::string());
                                }
                            }
                        }
                    }

                    if (npcId != -1 && !scriptText.empty())
                    {
                        s->loadNPC(objName, npcId, objX, objY, scriptText.c_str());
                    }
                    else
                    {
                        LOG_WARN("Unrecognized format for npc");
                    }
                }
                else if (utils::compareStrI(objType, "SCRIPT") == 0)
                {
                    std::string scriptFilename;
                    std::string scriptText;

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
                                std::string value = XML::getProperty(propertyNode, "name",
                                                                     std::string());
                                value = utils::toUpper(value);
                                if (utils::compareStrI(value, "FILENAME") == 0)
                                {
                                    scriptFilename = getObjectProperty(propertyNode,
                                                                       std::string());
                                    utils::trim(scriptFilename);
                                }
                                else if (utils::compareStrI(value, "TEXT") == 0)
                                {
                                    scriptText = getObjectProperty(propertyNode, "");
                                }
                            }
                        }
                    }

                    Script *s = composite->getScript();
                    if (!s)
                    {
                        std::string engineName="";
                        // Create a Lua context.
                        if (!scriptFilename.empty())
                        {
                            engineName=Script::determinateEngineOverFilename(scriptFilename);
                        }
                        else
                        {
                            engineName="lua"; //TODO Determinate Scripttype
                        }

                        s = Script::create(engineName);
                        composite->setScript(s);
                    }

                    if (!scriptFilename.empty())
                    {
                        s->loadFile(scriptFilename);
                    }
                    else if (!scriptText.empty())
                    {
                        const std::string name = "'" + objName + "'' in " + path;
                        s->load(scriptText.c_str(), name.c_str());
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
    ::tilesetFirstGids.clear();

    return map;
}

void MapReader::readLayer(xmlNodePtr node, Map *map)
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

    std::string encoding = XML::getProperty(node, "encoding", std::string());
    if (encoding == "base64")
    {
        // Read base64 encoded map file
        xmlNodePtr dataChild = node->xmlChildrenNode;
        if (!dataChild)
        {
            LOG_WARN("Corrupted layer.");
            return;
        }

        int len = strlen((const char *) dataChild->content) + 1;
        char *charData = new char[len + 1];
        const char *charStart = (const char *) dataChild->content;
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

        std::string compression =
            XML::getProperty(node, "compression", std::string());
        if (compression == "gzip" || compression == "zlib")
        {
            // Inflate the gzipped layer data
            char *inflated;
            unsigned inflatedSize;
            bool res = inflateMemory((char *)binData, binLen, inflated, inflatedSize);
            free(binData);

            if (!res)
            {
                LOG_WARN("Failed to decompress compressed layer");
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
    else if (encoding == "csv")
    {
        xmlNodePtr dataChild = node->xmlChildrenNode;
        if (!dataChild)
            return;

        const char *data = (const char*) xmlNodeGetContent(dataChild);
        std::string csv(data);

        size_t pos = 0;
        size_t oldPos = 0;

        while (oldPos != csv.npos)
        {
            pos = csv.find_first_of(",", oldPos);

            const int gid = atoi(csv.substr(oldPos, pos - oldPos).c_str());

            setTileWithGid(map, x, y, gid);

            x++;
            if (x == w)
            {
                x = 0;
                ++y;

                // When we're done, don't crash on too much data
                if (y == h)
                    break;
            }

            oldPos = pos + 1;
        }
    }
    else
    {

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
}

std::string MapReader::getObjectProperty(xmlNodePtr node,
                                         const std::string &def)
{
    if (xmlHasProp(node, BAD_CAST "value"))
    {
        return XML::getProperty(node, "value", def);
    }
    else if (const char *prop = (const char *)node->xmlChildrenNode->content)
    {
        return std::string(prop);
    }
    return std::string();
}

int MapReader::getObjectProperty(xmlNodePtr node, int def)
{
    int val = def;
    if (xmlHasProp(node, BAD_CAST "value"))
    {
        val = XML::getProperty(node, "value", def);
    }
    else if (const char *prop = (const char *)node->xmlChildrenNode->content)
    {
        val = atoi(prop);
    }
    return val;
}

void MapReader::setTileWithGid(Map *map, int x, int y, int gid)
{
    // Find the tileset with the highest firstGid below/eq to gid
    int set = gid;
    for (std::vector< int >::const_iterator i = ::tilesetFirstGids.begin(),
         i_end = ::tilesetFirstGids.end(); i != i_end; ++i)
    {
        if (gid < *i)
            break;

        set = *i;
    }

    if (gid != set)
        map->blockTile(x, y, BLOCKTYPE_WALL);
}
