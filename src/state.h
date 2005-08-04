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

#include <string>
#include <map>
#include "connectionhandler.h"
#include "being.h"
#include "map.h"
#include "utils/singleton.h"

namespace tmwserv
{

/**
 * State class contains all information/procedures associated with the game
 * state.
 */
class State : public utils::Singleton<State>
{
    friend class utils::Singleton<State>;

    State() throw() { }
    ~State() throw() { }

 public:
    /**
     * Beings on map
     *
     * The key/value pair conforms to:
     *   First  - map name
     *   Second - list of beings/players on the map
     *
     * NOTE: This could possibly be optimized by making first Being & second string. This will make many operations easier.
     */
    std::map<std::string, Beings> beings;

    /**
     * Items on map
     *
     * The key/value pair conforms to:
     *   First  - map name
     *   Second - Item ID
     */
    std::map<std::string, int> items;

    /**
     * Container for loaded maps.
     */
    std::map<std::string, Map*> maps;
    
    /**
     * Update game state (contains core server logic)
     */
    void update(ConnectionHandler &);
};

} // namespace tmwserv

#endif
