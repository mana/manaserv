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

#include "resourcemanager.h"
#include "game-server/map.hpp"
#include "game-server/mapreader.hpp"
#include "utils/base64.h"
#include "utils/logger.h"
#include "utils/xml.hpp"
#include "utils/zlib.hpp"

static std::vector< int > tilesetFirstGids;

static Map* readMap(xmlNodePtr node, std::string const &path);
static void readLayer(xmlNodePtr node, Map *map);
static void setTileWithGid(Map *map, int x, int y, int gid);

Map *MapReader::readMap(const std::string &filename)
{
    // Load the file through resource manager.
    ResourceManager *resman = ResourceManager::getInstance();
    int fileSize;
    char *buffer = (char *)resman->loadFile(filename, fileSize);

    if (buffer == NULL)
    {
        LOG_ERROR("Error: Map file not found (" << filename.c_str() << ")");
        return NULL;
    }

    // Inflate the gzipped map data.
    char *inflated;
    unsigned inflatedSize = 0;
    bool ret = inflateMemory(buffer, fileSize, inflated, inflatedSize);
    free(buffer);

    xmlDocPtr doc = NULL;

    if (ret)
    {
        doc = xmlParseMemory(inflated, inflatedSize);
        free(inflated);
    }

    if (!doc)
    {
        LOG_ERROR("Error while parsing map file (" << filename << ")!");
        return NULL;
    }

    Map *map = NULL;
    xmlNodePtr node = xmlDocGetRootElement(doc);

    // Parse the inflated map data.
    if (node && xmlStrEqual(node->name, BAD_CAST "map"))
    {
        map = ::readMap(node, filename);
    }
    else
    {
        LOG_ERROR("Error: Not a map file (" << filename << ")!");
    }

    xmlFreeDoc(doc);
    return map;
}

static Map *readMap(xmlNodePtr node, std::string const &path)
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

    // Load the tile data. Layers are assumed to be map size, with (0,0) as
    // origin.
    while (node != NULL)
    {
        if (xmlStrEqual(node->name, BAD_CAST "data"))
        {
            if (XML::getProperty(node, "encoding", std::string()) == "base64")
            {
                if (xmlHasProp(node, BAD_CAST "compression"))
                {
                    LOG_WARN("Warning: no layer compression supported!");
                    return;
                }

                // Read base64 encoded map file
                xmlNodePtr dataChild = node->xmlChildrenNode;
                if (!dataChild) continue;

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

                if (binData)
                {
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
                }
            }
            else
            {
                // Read plain XML map file
                xmlNodePtr n2 = node->xmlChildrenNode;

                while (n2 != NULL)
                {
                    if (xmlStrEqual(n2->name, BAD_CAST "tile") && y < h)
                    {
                        int gid = XML::getProperty(n2, "gid", -1);
                        setTileWithGid(map, x, y, gid);

                        if (++x == w)
                        {
                            x = 0;
                            ++y;
                        }
                    }

                    n2 = n2->next;
                }
            }

            // There can be only one data element
            break;
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

    map->setWalk(x, y, gid == set);
}
