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
 */

#ifndef _TMW_MAPMANAGER_H
#define _TMW_MAPMANAGER_H

#include <map>
#include <string>

class MapComposite;

namespace MapManager
{
    typedef std::map< int, MapComposite * > Maps;

    /**
     * Loads map reference file and prepares maps.
     */
    void initialize(const std::string &mapReferenceFile);

    /**
     * Destroy loaded maps.
     */
    void deinitialize();

    /**
     * Returns the requested map.
     *
     * @return the requested map, or NULL if no map with the given ID exists.
     */
    MapComposite *getMap(int mapId);

    /**
     *  Returns the requested map
     */
    MapComposite *getMap(const std::string &mapName);

    /**
     * Returns all the maps.
     */
    const Maps &getMaps();

    /**
     * Sets the activity status of the map.
     */
    void raiseActive(int mapId);
}

#endif
