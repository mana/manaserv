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

#include "map.h"

#include "utils/singleton.h"

namespace tmwserv
{


/**
 * MapManager loads/unloads maps
 */
class MapManager: public utils::Singleton<MapManager>
{
    // friend so that Singleton can call the constructor.
    friend class utils::Singleton<MapManager>;

    public:
        /**
         * Load the specified map
         */
        Map *loadMap(unsigned int mapId);

        /**
         * Unload the specified map
         */
        void unloadMap(unsigned int mapId);

        /**
         * Reload the specified map
         */
        void reloadMap(unsigned int mapId);

        /**
         * Return the requested map
         */
        Map *getMap(unsigned int mapId);

        /**
         * Check if a map was already loaded.
         */
        bool isLoaded(unsigned int mapId) const;

    protected:
        /**
         * Destructor.
         */
        ~MapManager(void)
            throw();

    private:
        // Hold all the loaded maps.
        std::map<unsigned int, Map *> maps;
};

} // namespace tmwserv

#endif
