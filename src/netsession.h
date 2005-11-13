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

#ifndef _TMWSERV_NETSESSION_H_
#define _TMWSERV_NETSESSION_H_

#include "netcomputer.h"
#include "connectionhandler.h"
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_net.h>
#include <map>

/**
 * Data communicated to a new listen thread. The <code>running</code> member is
 * set to <code>false</code> to tell the thread to stop listening.
 */
struct ListenThreadData
{
    IPaddress address;           /**< Includes the port to listen to. */
    TCPsocket socket;            /**< The socket that's been opened. */
    SDL_Thread *thread;          /**< The thread, ignored by thread itself. */
    ConnectionHandler *handler;  /**< Handler for events. */
    bool running;                /**< Wether to keep listening. */
};

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
         *
         * This method opens a socket on the given port and starts a new thread
         * that will handle listening for new connections and incoming data
         * over this port. The connection handler will need to be thread safe.
         */
        void startListen(ConnectionHandler *handler, Uint16 port);

        /**
         * Stop listening for connections and disconnect any connected clients.
         * This is done by signalling the listening thread to stop running, and
         * closing the socket when it stopped. 
         */
        void stopListen(Uint16 port);

        /**
         * Connect to another network session.
         */
        NetComputer *connect(const std::string &ip, Uint16 port);

    private:
        /**
         * The list of ports we're listening to and their associated thread
         * data, including the connection handler and wether to keep listening.
         */
        std::map<Uint16, ListenThreadData*> listeners;

        //  Other information we need to keep:
        //
        //  - The list of clients that connected and their associated net
        //    computers.
        //  - The list of servers we connected to and their associated net
        //    computers.
};

#endif
