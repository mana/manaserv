/*
 *  The Mana World Server
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

#ifndef _TMW_SERVER_STATE_
#define _TMW_SERVER_STATE_

#include <map>

#include "being.h"

#include "utils/singleton.h"

class ConnectionHandler;

namespace tmwserv
{
    class Map;

/**
 * Combined map/entity structure
 */
struct MapComposite {
    /**
     * Default constructor
     */
    MapComposite() : map(NULL) { }

    /**
     * Actual map
     */
    Map *map;

    /**
     * Beings located on the map
     */
    Beings beings;

    /**
     * Items located on the map
     */
    Objects objects;
};

/**
 * State class contains all information/procedures associated with the game
 * world's state.
 */
class State : public utils::Singleton<State>
{
    friend class utils::Singleton<State>;

    State() throw();
    ~State() throw();

    /**
     * List of maps
     */
    std::map<unsigned int, MapComposite> maps;

 public:

    /**
     * Update game state (contains core server logic)
     */
    void update();

    /**
     * Add being to game world at specified map
     */
    void addBeing(BeingPtr beingPtr, const unsigned int mapId);

    /**
     * Remove being from game world
     */
    void removeBeing(BeingPtr beingPtr);

    /**
     * Check to see if a map exists in game world
     */
    bool mapExists(const unsigned int mapId);

    /**
     * Check if being exists in game world already
     */
    bool beingExists(BeingPtr beingPtr);

    /**
     * Load map into game world
     */
    bool loadMap(const unsigned int mapId);

    /**
     * Add object to the map
     */
    void addObject(ObjectPtr objectPtr, const unsigned int mapId);

    /**
     * Remove an object from the map
     */
    void removeObject(ObjectPtr objectPtr);

    /**
     * Find out whether an object exists in the game world or not
     */
    bool objectExists(const ObjectPtr objectPtr);

    /**
     * Find map player in world is on
     */
    const unsigned int findPlayer(BeingPtr being);

    /**
     * Find map object in world is on
     */
    const unsigned int findObject(ObjectPtr objectPtr);
};

} // namespace tmwserv

#endif
