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

#ifndef _TMW_SERVER_CONNECTIONHANDLER_

#include "netcomputer.h"
#include "packet.h"
#include <map>

/**
 * This class represents the connection handler interface. The connection
 * handler will respond to connect/reconnect/disconnect events and handle
 * incoming messages, passing them on to registered message handlers.
 */
class ConnectionHandler
{
    public:
        /**
         * Called when a computer connects to a network session.
         */
        void computerConnected(NetComputer *computer);

        /**
         * Called when a computer reconnects to a network session.
         */
        void computerReconnected(NetComputer *computer);

        /**
         * Called when a computer disconnects from a network session.
         *
         * <b>Note:</b> After returning from this method the NetComputer
         *              reference is no longer guaranteed to be valid.
         */
        void computerDisconnected(NetComputer *computer);

        /**
         * Called when a computer sends a packet to the network session.
         */
        void receivePacket(NetComputer *computer, Packet *packet);

        /**
         * Registers a message handler to handle a certain message type.
         */
        void registerHandler(unsigned int msgId, MessageHandler *handler);

    private:
        std::map<unsigned int, MessageHandler*> handlers;
};

#endif
