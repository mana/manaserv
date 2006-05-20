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

#ifndef _TMWSERV_CONNECTIONHANDLER_H_
#define _TMWSERV_CONNECTIONHANDLER_H_

#include <list>
#include <map>
#include <enet/enet.h>

#include "being.h"

#define IN_BUFFER_SIZE   8192

class MessageHandler;
class MessageOut;
class NetComputer;

/**
 * Data related to a connected client. This includes the buffer for incoming
 * messages and the related socket.
 */
class ClientData
{
    public:
        ClientData();

        //TCPsocket sock;           /**< The socket used for communication */

        int inp;                  /**< The amount of data in the in buffer */
        char in[IN_BUFFER_SIZE];  /**< The in buffer for incoming messages */
};

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
         */
        void process();

        /**
         * Called when a computer sends a packet to the network session.
         */
        //void receivePacket(NetComputer *computer, Packet *packet);

        /**
         * Registers a message handler to handle a certain message type.
         */
        void registerHandler(unsigned int msgId, MessageHandler *handler);

        /**
         * Send packet to every client, used for announcements.
         */
        void sendToEveryone(MessageOut &);

        /**
         * Return the number of connected clients.
         */
        unsigned int getClientNumber();

    private:
        ENetAddress address;         /**< Includes the port to listen to. */
        ENetHost *host;              /**< The host that listen for connections. */

        typedef std::map< unsigned int, MessageHandler * > HandlerMap;
        HandlerMap handlers;

    protected:
        /**
         * Called when a computer connects to the server. Initialize
         * an object derived of NetComputer.
         */
        virtual NetComputer *computerConnected(ENetPeer *) = 0;

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

        typedef std::list<NetComputer*> NetComputers;
        /**
         * A list of pointers to the client structures created by
         * computerConnected.
         */
        NetComputers clients;
};

/**
 * Temporary placeholder until the connection handlers have been split.
 */
class ClientConnectionHandler: public ConnectionHandler
{
    public:
        /**
         * Send packet to client with matching BeingPtr
         */
        void sendTo(tmwserv::BeingPtr, MessageOut &);

        /**
         * Send packet to client with matching Being name
         */
        void sendTo(std::string name, MessageOut &);

        /**
         * Send packet to every client around the client on screen.
         */
        void sendAround(tmwserv::BeingPtr, MessageOut &);

        /**
         * Send packet to every client in a registered channel.
         */
        void sendInChannel(short channelId, MessageOut &);

        /**
         * tells a list of user they're leaving a channel.
         */
        void makeUsersLeaveChannel(const short channelId);

        /**
         * tells a list of user about an event in a chatchannel about a player.
         */
        void warnUsersAboutPlayerEventInChat(const short channelId,
                                             const std::string& userName,
                                             const char eventId);

    protected:
        virtual NetComputer *computerConnected(ENetPeer *);
        virtual void computerDisconnected(NetComputer *);
};

extern ClientConnectionHandler *connectionHandler;

#endif
