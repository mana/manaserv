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

#ifndef _TMWSERV_NETCOMPUTER_H_
#define _TMWSERV_NETCOMPUTER_H_

#include <iosfwd>
#include <queue>

#include <enet/enet.h>

#include "account.h"
#include "being.h"

// Forward declaration
class ConnectionHandler;
class MessageOut;

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
        NetComputer(ConnectionHandler *handler, ENetPeer *peer);

        /**
         * Destructor
         */
        virtual ~NetComputer() {}

        /**
         * Returns <code>true</code> if this computer is disconnected.
         */
        //bool
        //isDisconnected();

        /**
         * Disconnects the computer from the server.
         */
        void
        disconnect(const std::string &reason);

        /**
         * Queues a message for sending to a client.
         *
         * TODO: Add support for reliable and maybe also channels now that we
         * TODO: have ENet.
         */
        void
        send(const MessageOut &msg);
        //void send(Packet *p, bool reliable = true);

    private:
        ConnectionHandler *mHandler;  /**< Connection handler */
        ENetPeer *mPeer;              /**< Client peer */
};

#endif
