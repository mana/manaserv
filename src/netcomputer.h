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

#ifndef _TMW_SERVER_NETCOMPUTER_
#define _TMW_SERVER_NETCOMPUTER_

#include "packet.h"
#include <SDL_net.h>
#include <string>
#include <queue>
#include <list>

// Forward declaration
class ConnectionHandler;

/**
 * This class represents a known computer on the network. For example a
 * connected client or a server we're connected to.
 */
class NetComputer
{
    public:
        /**
         * Constructor.
         */
        NetComputer(ConnectionHandler *handler);

        /**
         * Destructor
         */
        ~NetComputer();

        /**
         * Returns <code>true</code> if this computer is disconnected.
         */
        bool isDisconnected();

        /**
         * Disconnects the computer from the server.
         */
        void disconnect(const std::string &reason);

        /**
         * Queues (FIFO) a packet for sending to a client.
         *
         * Note: When we'd want to allow communication through UDP, we could
         *  introduce the reliable argument, which would cause a UDP message
         *  to be sent when set to false.
         */
        void send(const Packet *p);
        //void send(Packet *p, bool reliable = true);

        /**
         * Get next message
         */
        Packet *front();

        /**
         * Number of messages in queue
         */
        unsigned int size() { return queue.size(); }

    private:
        ConnectionHandler *handler;

        std::queue<Packet*> queue; /**< Message Queue (FIFO) */
};

#endif
