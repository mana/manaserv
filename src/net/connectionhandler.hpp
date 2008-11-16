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
 */

#ifndef _TMWSERV_CONNECTIONHANDLER_H_
#define _TMWSERV_CONNECTIONHANDLER_H_

#include <list>
#include <enet/enet.h>

class MessageIn;
class MessageOut;
class NetComputer;

/**
 * This class represents the connection handler interface. The connection
 * handler will respond to connect/reconnect/disconnect events and handle
 * incoming messages, passing them on to registered message handlers.
 */
class ConnectionHandler
{
    public:
        virtual ~ConnectionHandler() {}

        /**
         * Open the server socket.
         */
        bool startListen(enet_uint16 port);

        /**
         * Disconnect all the clients and close the server socket.
         */
        void stopListen();

        /**
         * Process outgoing messages and listen to the server socket for
         * incoming messages and new connections.
         *
         * @timeout an optional timeout in milliseconds to wait for something
         *          to happen when there is nothing to do
         */
        virtual void process(enet_uint32 timeout = 0);

        /**
         * Process outgoing messages.
         */
        void flush();

        /**
         * Called when a computer sends a packet to the network session.
         */
        //void receivePacket(NetComputer *computer, Packet *packet);

        /**
         * Send packet to every client, used for announcements.
         */
        void sendToEveryone(const MessageOut &msg);

        /**
         * Return the number of connected clients.
         */
        unsigned int getClientNumber();

    private:
        ENetAddress address;      /**< Includes the port to listen to. */
        ENetHost *host;           /**< The host that listen for connections. */

    protected:
        /**
         * Called when a computer connects to the server. Initialize
         * an object derived of NetComputer.
         */
        virtual NetComputer *computerConnected(ENetPeer *peer) = 0;

        /**
         * Called when a computer reconnects to the server.
         */
        //virtual NetComputer *computerReconnected(ENetPeer *) = 0;

        /**
         * Called when a computer disconnects from the server.
         *
         * <b>Note:</b> After returning from this method the NetComputer
         *              reference is no longer guaranteed to be valid.
         */
        virtual void computerDisconnected(NetComputer *) = 0;

        /**
         * Called when a message is received.
         */
        virtual void processMessage(NetComputer *, MessageIn &) = 0;

        typedef std::list<NetComputer*> NetComputers;
        /**
         * A list of pointers to the client structures created by
         * computerConnected.
         */
        NetComputers clients;
};

#endif
