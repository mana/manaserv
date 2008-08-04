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

#ifndef _INCLUDED_MAPREADER_H
#define _INCLUDED_MAPREADER_H

#include <string>
#include <vector>

#include <libxml/tree.h>

class Map;
class MapComposite;
class Thing;

/**
 * Reader for XML map files (*.tmx)
 */
class MapReader
{
    public:
        /**
         * Read an XML map from a file.
         */
        static void readMap(const std::string &filename, MapComposite *composite);

    private:
        /**
         * Read an XML map from a parsed XML tree, and populate things with objects
         * in that map.
         */
        static Map* readMap(xmlNodePtr node, std::string const &path,
                            MapComposite *composite, std::vector<Thing *> &things);

        /**
         * Reads a map layer and adds it to the given map.
         */
        static void readLayer(xmlNodePtr node, Map *map);

        /**
         * Get the string value from the given object property node.
         */
        static std::string getObjectProperty(xmlNodePtr node, const std::string &def);

        /**
         * Get the integer value from the given object property node.
         */
        static int getObjectProperty(xmlNodePtr node, int def);

        static void setTileWithGid(Map *map, int x, int y, int gid);
};

#endif
