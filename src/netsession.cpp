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


int startListenThread(void *data)
{
    // So who will do the actual listening now?
    return 0;
}


NetSession::NetSession()
{
}

NetSession::~NetSession()
{
    // Stop listening to any ports
}

void NetSession::startListen(ConnectionHandler *handler, Uint16 port)
{
    // Here we will probably need the creation of a listening thread, which
    // will call connect/disconnect events on the given ConnectionHandler and
    // will cut incoming data into Packets and send them there too.

    ListenThreadData *data = new ListenThreadData;

    data->address.host = INADDR_ANY;
    data->address.port = port;
    data->handler = handler;
    data->running = true;
    data->socket = SDLNet_TCP_Open(&data->address);

    if (!data->socket) {
        printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
        exit(3);
    }

    data->thread = SDL_CreateThread(startListenThread, data);

    if (data->thread == NULL) {
        printf("SDL_CreateThread: %s\n", SDL_GetError());
        exit(5);
    }

    listeners[port] = data;
}

void NetSession::stopListen(Uint16 port)
{
    std::map<Uint16, ListenThreadData*>::iterator threadDataI;
    threadDataI = listeners.find(port);

    if (threadDataI != listeners.end())
    {
        ListenThreadData *data = (*threadDataI).second;

        // Tell listen thread to stop running
        data->running = false;

        // Wait for listen thread to stop and close socket
        // Note: Somewhere in this process the ConnectionHandler should receive
        //       disconnect notifications about all the connected clients.
        SDL_WaitThread(data->thread, NULL);
        SDLNet_TCP_Close(data->socket);
    }
    else
    {
        printf("NetSession::stopListen() not listening to port %d!\n", port);
    }
}

NetComputer *NetSession::connect(const std::string &host, Uint16 port)
{
    // Try to connect to given host:port, and return NetComputer objects that
    // can be used to send messages that way, or NULL when failing to connect.
    //
    // An asynchroneous wrapper could be created around this method.

    IPaddress address;
    
    if (!SDLNet_ResolveHost(&address, host.c_str(), port))
    {
        TCPsocket tcpsock = SDLNet_TCP_Open(&address);
        if (!tcpsock) {
            printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
            exit(3);
        }

        // return computer;
    }
    else {
        printf("SDLNet_ResolveHost: Could not resolve %s\n", host.c_str());
        exit(4);
    }

    return NULL;
}
