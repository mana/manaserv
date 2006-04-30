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

#include "netsession.h"

#include "connectionhandler.h"

#include "utils/logger.h"

/**
 * This function is the new thread created to listen to a server socket. It
 * immediately passes control over to the connection handler instance that will
 * deal with incoming connections and data.
 */
void *startListenThread(void *data)
{
    ListenThreadData *ltd = (ListenThreadData*)data;
    ltd->handler->startListen(ltd);
    pthread_exit(NULL);
}


NetSession::NetSession()
{
}

NetSession::~NetSession()
{
    // Stop listening to any ports
}

void NetSession::startListen(ConnectionHandler *handler, enet_uint16 port)
{
    // Here we will probably need the creation of a listening thread, which
    // will call connect/disconnect events on the given ConnectionHandler and
    // will cut incoming data into Packets and send them there too.

    ListenThreadData *data = new ListenThreadData();

    data->handler = handler;
    data->running = true;

    ENetHost *server;

    // Bind the server to the default localhost.
    data->address.host = ENET_HOST_ANY;
    data->address.port = port;

    server = enet_host_create(&data->address /* the address to bind the server host to */,
                              MAX_CLIENTS    /* allow up to MAX_CLIENTS clients and/or outgoing connections */,
                              0              /* assume any amount of incoming bandwidth */,
                              0              /* assume any amount of outgoing bandwidth */);
    if (server == NULL)
    {
        LOG_ERROR("Unable to create an ENet server host.", 0);
        exit(3);
    }
    
    data->host = server;
    
    // Start the listening thread
    int rc = pthread_create(&data->thread, NULL,
                            startListenThread, (void *)data);
    if (rc) {
        LOG_ERROR("pthread_create: " << rc, 0);
        exit(4);
    }
    
    listeners[port] = data;
}

void NetSession::stopListen(enet_uint16 port)
{
    std::map<enet_uint16, ListenThreadData*>::iterator threadDataI;
    threadDataI = listeners.find(port);

    if (threadDataI != listeners.end())
    {
        ListenThreadData *data = (*threadDataI).second;

        // Tell listen thread to stop running
        data->running = false;

        // Wait for listen thread to stop and close socket
        // Note: Somewhere in this process the ConnectionHandler should receive
        //       disconnect notifications about all the connected clients.
        enet_host_destroy(data->host);
        delete data;
        listeners.erase(threadDataI);
    }
    else
    {
        LOG_WARN("NetSession::stopListen() not listening to port %d!\n", port);
    }
}

NetComputer *NetSession::connect(const std::string &host, enet_uint16 port)
{
    // Try to connect to given host:port, and return NetComputer objects that
    // can be used to send messages that way, or NULL when failing to connect.
    //
    // An asynchroneous wrapper could be created around this method.
    
    ENetHost *client;

    client = enet_host_create(NULL, 1, 0, 0);

    if (client == NULL)
    {
        LOG_ERROR("Unable to create an ENet client host.", 0);
        exit(3);
    }
    
    ENetAddress address;
    ENetEvent event;
    ENetPeer *peer;

    // Connect to host:port.
    enet_address_set_host(&address, host.c_str());
    address.port = port;

    // Initiate the connection, allocating the channel 0.
    peer = enet_host_connect(client, &address, 1);

    if (peer == NULL)
    {
       LOG_ERROR("No available peer for initiating an ENet connection.", 0);
       exit(4);
    }

    // Wait up to 5 seconds for the connection attempt to succeed.
    if (enet_host_service (client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        LOG_INFO("Connection succeeded.", 0);
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);
        LOG_ERROR("Connection failed.", 0);
        exit(5);
    }

    return NULL;
}
