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
 */

#ifndef _TMWSERV_CONNECTION_H_
#define _TMWSERV_CONNECTION_H_

#include <string>
#include <enet/enet.h>

class MessageIn;
class MessageOut;
class BandwidthMonitor;

/**
 * A point-to-point connection to a remote host. The remote host can use a
 * ConnectionHandler to handle this incoming connection.
 */
class Connection
{
    public:
        Connection();
        virtual ~Connection() {}

        /**
         * Connects to the given host/port and waits until the connection is
         * established. Returns false if it fails to connect.
         */
        bool start(const std::string &, int);

        /**
         * Disconnects.
         */
        void stop();

        /**
         * Returns whether the connection is established or not.
         */
        bool isConnected() const;

        /**
         * Sends a message to the remote host.
         */
        void send(const MessageOut &msg, bool reliable = true,
                  unsigned channel = 0);

        /**
         * Dispatches received messages to processMessage.
         */
        void process();

    protected:
        /**
         * Processes a single message from the remote host.
         */
        virtual void processMessage(MessageIn &) = 0;

    private:
        ENetPeer *mRemote;
        ENetHost *mLocal;
};

#endif
