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

#include "connectionhandler.h"
#include "netsession.h"
#include "log.h"

#ifdef SCRIPT_SUPPORT
#include "script.h"
#endif

#define MAX_CLIENTS 1024

ClientData::ClientData():
    inp(0)
{
}

ConnectionHandler::ConnectionHandler()
{
}

void ConnectionHandler::startListen(ListenThreadData *ltd)
{
    // Allocate a socket set
    SDLNet_SocketSet set = SDLNet_AllocSocketSet(MAX_CLIENTS);
    if (!set) {
        logger->log("SDLNet_AllocSocketSet: %s", SDLNet_GetError());
        exit(1);
    }

    // Add the server socket to the socket set
    if (SDLNet_TCP_AddSocket(set, ltd->socket) < 0) {
        logger->log("SDLNet_AddSocket: %s", SDLNet_GetError());
        exit(1);
    }

    // Keep checking for socket activity while running
    while (ltd->running)
    {
        int numready = SDLNet_CheckSockets(set, 100);

        if (numready == -1)
        {
            printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
            logger->log("SDLNet_CheckSockets: %s", SDLNet_GetError());
            // When this is a system error, perror may help us
            perror("SDLNet_CheckSockets");
        }
        else if (numready > 0)
        {
            logger->log("%d sockets with activity!\n", numready);

            // Check server socket
            if (SDLNet_SocketReady(ltd->socket))
            {
                TCPsocket client = SDLNet_TCP_Accept(ltd->socket);
                if (client)
                {
                    // Add the client socket to the socket set
                    if (SDLNet_TCP_AddSocket(set, client) < 0) {
                        logger->log("SDLNet_AddSocket: %s", SDLNet_GetError());
                    }
                    else {
                        NetComputer *comp = new NetComputer(this);
                        clients[comp] = client;
                        computerConnected(comp);
                        logger->log("%d clients connected", clients.size());
                    }
                }
            }

            // Check client sockets
            std::map<NetComputer*, TCPsocket>::iterator i;
            for (i = clients.begin(); i != clients.end(); )
            {
                NetComputer *comp = (*i).first;
                TCPsocket s = (*i).second;

                if (SDLNet_SocketReady(s))
                {
                    char buffer[1024];
                    int result = SDLNet_TCP_Recv(s, buffer, 1024);
                    if (result <= 0)
                    {
                        SDLNet_TCP_DelSocket(set, s);
                        SDLNet_TCP_Close(s);
                        computerDisconnected(comp);
                        delete comp;
                        comp = NULL;
                    }
                    else
                    {
                        // Copy the incoming data to the in buffer of this
                        // client
                        buffer[result] = 0;
                        logger->log("Received %s", buffer);
#ifdef SCRIPT_SUPPORT
                        //script->message(buffer);
#endif
                    }
                }

                // Traverse to next client, possibly deleting current
                if (comp == NULL) {
                    std::map<NetComputer*, TCPsocket>::iterator ii = i;
                    ii++;
                    clients.erase(i);
                    i = ii;
                }
                else {
                    i++;
                }
            }
        }
    }

    // - Disconnect all clients (close sockets)

    SDLNet_FreeSocketSet(set);
}

void ConnectionHandler::computerConnected(NetComputer *comp)
{
    logger->log("A client connected!");
}

void ConnectionHandler::computerDisconnected(NetComputer *comp)
{
    logger->log("A client disconnected!");
}

void ConnectionHandler::registerHandler(
        unsigned int msgId, MessageHandler *handler)
{
    handlers[msgId] = handler;
}
