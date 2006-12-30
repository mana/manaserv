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

#ifndef _TMW_MAPMANAGER_H
#define _TMW_MAPMANAGER_H

#include <map>
#include <string>

class Map;

struct LoadedMap
{
    std::string fileName;
    Map *map;
};

/**
 * MapManager loads/unloads maps
 */
class MapManager
{
    public:
        typedef std::map< unsigned, LoadedMap > Maps;

        /**
         * Constructor (loads map reference file).
         */
        MapManager(std::string const &);

        /**
         * Returns the requested map.
         */
        Map *getMap(unsigned);

        /**
         * Returns the requested map name.
         */
        std::string getMapName(unsigned);

        /**
         * Returns all the maps.
         */
        Maps const &getMaps() { return maps; }

        /**
         * Destructor.
         */
        ~MapManager();

    private:
        Maps maps;
};

extern MapManager *mapManager;

#endif
