/*
 *  The Mana World Server
 *  Copyright 2006 The Mana World Development Team
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

#ifndef _TMWSERV_SERVERHANDLER_H_
#define _TMWSERV_SERVERHANDLER_H_

#include <iosfwd>
#include <string>

class Character;

namespace GameServerHandler
{
    /**
     * Creates a connection handler and starts listening on given port.
     */
    bool initialize(int port);

    /**
     * Stops listening to messages and destroys the connection handler.
     */
    void deinitialize();

    /**
     * Returns the information a client needs to connect to the game server
     * corresponding to the given map ID.
     */
    bool getGameServerFromMap(int, std::string &address, int &port);

    /**
     * Warns a game server about a soon-to-connect client.
     */
    void registerClient(std::string const &token, Character *);

    /**
     * Dumps per-server statistics into given stream
     */
    void dumpStatistics(std::ostream &);

    /**
     * Processes messages received by the connection handler.
     */
    void process();
}

#endif
