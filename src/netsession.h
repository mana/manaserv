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

#ifndef _TMW_SERVER_NETSESSION_

#include "netcomputer.h"
#include "connectionhandler.h"

/**
 * This class represents a network session. It implements listening for
 * connections from and connecting to other computers.
 */
class NetSession
{
    public:
        /**
         * Constructor.
         */
        NetSession();

        /**
         * Destructor.
         */
        ~NetSession();

        /**
         * Start listening for connections and notify the given connection
         * handler about events.
         */
        void startListen(ConnectionHandler *handler, int port);

        /**
         * Stop listening for connections and disconnect any connected clients.
         */
        void stopListen(int port);

        /**
         * Connect to another network session.
         */
        NetComputer *connect(const std::string &ip, int port);

    private:
        //  This class probably needs to keep information about:
        //
        //  - The list of ports we're listening to and their associated
        //    connection handlers.
        //  - The list of clients that connected and their associated net
        //    computers.
        //  - The list of servers we connected to and their associated net
        //    computers.
};

#endif
