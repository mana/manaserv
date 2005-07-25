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

#include "utils/logger.h"
#include "utils/base64.h"
#include "mapreader.h"
#include "resourcemanager.h"

#include <cassert>

#include <zlib.h>
#include <iostream>

namespace tmwserv
{


const unsigned int DEFAULT_TILE_WIDTH = 32;
const unsigned int DEFAULT_TILE_HEIGHT = 32;

// MSVC libxml2 at the moment doesn't work right when using MinGW, missing this
// function at link time.
#ifdef WIN32
#undef xmlFree
#define xmlFree(x) ;
#endif

/**
 * Inflates either zlib or gzip deflated memory. The inflated memory is
 * expected to be freed by the caller.
 */
int
inflateMemory(
        unsigned char *in, unsigned int inLength,
        unsigned char *&out, unsigned int &outLength)
{
    int bufferSize = 256 * 1024;
    int ret;
    z_stream strm;

    out = (unsigned char*)malloc(bufferSize);

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = in;
    strm.avail_in = inLength;
    strm.next_out = out;
    strm.avail_out = bufferSize;

    ret = inflateInit2(&strm, 15 + 32);

    if (ret != Z_OK)
        return ret;

    do
    {
        if (strm.next_out == NULL)
        {
            inflateEnd(&strm);
            return Z_MEM_ERROR;
        }

        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);

        switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
        }

        if (ret != Z_STREAM_END)
        {
            out = (unsigned char*)realloc(out, bufferSize * 2);

            if (out == NULL)
            {
                inflateEnd(&strm);
                return Z_MEM_ERROR;
            }

            strm.next_out = out + bufferSize;
            strm.avail_out = bufferSize;
            bufferSize *= 2;
        }
    }
    while (ret != Z_STREAM_END);
    assert(strm.avail_in == 0);

    outLength = bufferSize - strm.avail_out;
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

std::vector<Tileset*> MapReader::tilesets;

Map *MapReader::readMap(const std::string &filename)
{
    // Load the file through resource manager
    ResourceManager *resman = ResourceManager::getInstance();
    int fileSize;
    void *buffer = resman->loadFile(filename, fileSize);

    if (buffer == NULL)
    {
        LOG_ERROR("Map file not found (" << filename.c_str() << ")");
        return NULL;
    }

    // Inflate the gzipped map data
    unsigned char *inflated;
    unsigned int inflatedSize = 0;
    int ret = inflateMemory((unsigned char*)buffer, fileSize,
                            inflated, inflatedSize);
    free(buffer);

    if (ret == Z_MEM_ERROR)
    {
        LOG_ERROR("Error: Out of memory while decompressing map data!");
        return NULL;
    }
    else if (ret == Z_VERSION_ERROR)
    {
        LOG_ERROR("Error: Incompatible zlib version!");
        return NULL;
    }
    else if (ret == Z_DATA_ERROR)
    {
        LOG_ERROR("Error: Incorrect zlib compressed data!");
        return NULL;
    }
    else if (ret != Z_OK || inflated == NULL)
    {
        LOG_ERROR("Error: Unknown error while decompressing map data!");
        return NULL;
    }

    xmlDocPtr doc = xmlParseMemory((char*)inflated, inflatedSize);
    free(inflated);

    // Parse the inflated map data
    if (doc) {
        xmlNodePtr node = xmlDocGetRootElement(doc);

        if (!node || !xmlStrEqual(node->name, BAD_CAST "map")) {
            LOG_ERROR("Error: Not a map file (" << filename << ")!");
            return NULL;
        }

        return readMap(node, filename);
        xmlFreeDoc(doc);
    } else {
        LOG_ERROR("Error while parsing map file (" << filename << ")!");
    }

    return NULL;
}

Map* MapReader::readMap(xmlNodePtr node, const std::string &path)
{
    xmlChar *prop;

    // Take the filename off the path
    std::string pathDir = path.substr(0, path.rfind("/") + 1);

    prop = xmlGetProp(node, BAD_CAST "version");
    xmlFree(prop);

    int w = getProperty(node, "width", 0);
    int h = getProperty(node, "height", 0);
    // We only support tile width of 32 at the moment
    //int tilew = getProperty(node, "tilewidth", DEFAULT_TILE_WIDTH);
    //int tileh = getProperty(node, "tileheight", DEFAULT_TILE_HEIGHT);
    int layerNr = 0;
    Map* map = new Map(w, h);

    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
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
        else if (xmlStrEqual(node->name, BAD_CAST "tileset"))
        {
            Tileset *tileset = readTileset(node, pathDir, map);
            if (tileset) {
                tilesets.push_back(tileset);
            }
        }
        else if (xmlStrEqual(node->name, BAD_CAST "layer"))
        {
            LOG_INFO("- Loading layer " << layerNr);
            readLayer(node, map, layerNr);
            layerNr++;
        }
    }

    // Clean up tilesets
    // TODO: Dereference them somewhere
    tilesets.clear();

    return map;
}

void MapReader::readLayer(xmlNodePtr node, Map *map, int layer)
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
            xmlChar *encoding = xmlGetProp(node, BAD_CAST "encoding");
            xmlChar *compression = xmlGetProp(node, BAD_CAST "compression");

            if (encoding && xmlStrEqual(encoding, BAD_CAST "base64"))
            {
                xmlFree(encoding);

                if (compression) {
                    LOG_WARN("Warning: no layer compression supported!");
                    xmlFree(compression);
                    return;
                }

                // Read base64 encoded map file
                xmlNodePtr dataChild = node->xmlChildrenNode;
                if (!dataChild) continue;

                int len = strlen((const char*)dataChild->content) + 1;
                unsigned char *charData = new unsigned char[len + 1];
                const char *charStart = (const char*)dataChild->content;
                unsigned char *charIndex = charData;

                while (*charStart) {
                    if (*charStart != ' ' && *charStart != '\t' &&
                            *charStart != '\n')
                    {
                        *charIndex = *charStart;
                        charIndex++;
                    }
                    charStart++;
                }
                *charIndex = '\0';

                int binLen;
                unsigned char *binData =
                    php_base64_decode(charData, strlen((char*)charData),
                            &binLen);

                delete[] charData;

                if (binData) {
                    for (int i = 0; i < binLen - 3; i += 4) {
                        int gid = binData[i] |
                            binData[i + 1] << 8 |
                            binData[i + 2] << 16 |
                            binData[i + 3] << 24;

                        setTileWithGid(map, x, y, layer, gid);

                        x++;
                        if (x == w) {x = 0; y++;}
                    }
                    free(binData);
                }
            }
            else {
                // Read plain XML map file
                xmlNodePtr n2 = node->xmlChildrenNode;

                while (n2 != NULL)
                {
                    if (xmlStrEqual(n2->name, BAD_CAST "tile") && y < h)
                    {
                        int gid = getProperty(n2, "gid", -1);
                        setTileWithGid(map, x, y, layer, gid);

                        x++;
                        if (x == w) {x = 0; y++;}
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

Tileset*
MapReader::readTileset(xmlNodePtr node, const std::string &path, Map *map)
{
    if (xmlHasProp(node, BAD_CAST "source")) {
        LOG_WARN("Warning: External tilesets not supported yet.");
        return NULL;
    }

    int firstGid = getProperty(node, "firstgid", 0);
    int tw = getProperty(node, "tilewidth", map->getTileWidth());
    int th = getProperty(node, "tileheight", map->getTileHeight());

    return new Tileset(tw, th, firstGid);
}

int MapReader::getProperty(xmlNodePtr node, const char* name, int def)
{
    xmlChar *prop = xmlGetProp(node, BAD_CAST name);
    if (prop) {
        int val = atoi((char*)prop);
        xmlFree(prop);
        return val;
    }
    else {
        return def;
    }
}

Tileset *MapReader::getTilesetWithGid(int gid)
{
    std::vector<Tileset*>::iterator i;
    Tileset *set = NULL;

    // Find the tileset with the highest firstGid below/eq to gid
    for (i = tilesets.begin(); i != tilesets.end(); ++i)
    {
        if ((*i)->getFirstGid() <= gid) {
            set = (*i);
        }
        else {
            break;
        }
    }

    return set;
}

void MapReader::setTileWithGid(Map *map, int x, int y, int layer, int gid)
{
    // Only load obstacle layer
    if (layer == 3)
    {
        Tileset *set = getTilesetWithGid(gid);
        map->setWalk(x, y, !set || (gid - set->getFirstGid() == 0));
    }
}

} // namespace tmwserv
