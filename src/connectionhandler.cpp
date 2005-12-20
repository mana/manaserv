/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */


#include <iostream>
#include <vector>
#include <sstream>
#include <SDL_net.h>

#include "connectionhandler.h"
#include "netsession.h"
#include "utils/logger.h"

#ifdef SCRIPT_SUPPORT
#include "script.h"
#endif

#define MAX_CLIENTS 1024

/**
 * TEMPORARY
 * Split a string into a std::vector delimiting elements by 'split'. This
 * function could be used for ASCII message handling (as we do not have
 * a working client yet, using ASCII allows tools like Netcat to be used
 * to test server functionality).
 *
 * This function may seem unoptimized, except it is this way to allow for
 * thread safety.
 */
std::vector<std::string>
stringSplit(const std::string &str,
            const std::string &split)
{
    std::vector<std::string> result; // temporary result
    unsigned int i;
    unsigned int last = 0;

    // iterate through string
    for (i = 0; i < str.length(); i++)
    {
        if (str.compare(i, split.length(), split.c_str(), split.length()) == 0)
        {
            result.push_back(str.substr(last, i - last));
            last = i + 1;
        }
    }

    // add remainder of string
    if (last < str.length()) {
        result.push_back(str.substr(last, str.length()));
    }

    return result;
}

/**
 * Convert a IP4 address into its string representation
 */
std::string
ip4ToString(unsigned int ip4addr)
{
    std::stringstream ss;
    ss << (ip4addr & 0x000000ff) << "."
       << ((ip4addr & 0x0000ff00) >> 8) << "."
       << ((ip4addr & 0x00ff0000) >> 16) << "."
       << ((ip4addr & 0xff000000) >> 24);
    return ss.str();
}

////////////////

ClientData::ClientData():
    inp(0)
{
}

ConnectionHandler::ConnectionHandler()
{
}

void
ConnectionHandler::startListen(ListenThreadData *ltd)
{
    // Allocate a socket set
    SDLNet_SocketSet set = SDLNet_AllocSocketSet(MAX_CLIENTS);
    if (!set) {
        LOG_FATAL("SDLNet_AllocSocketSet: " << SDLNet_GetError(), 0)
        exit(1);
    }

    // Add the server socket to the socket set
    if (SDLNet_TCP_AddSocket(set, ltd->socket) < 0) {
        LOG_FATAL("SDLNet_AddSocket: " << SDLNet_GetError(), 0)
        exit(1);
    }

    // Keep checking for socket activity while running
    while (ltd->running) {
        int numready = SDLNet_CheckSockets(set, 100);

        if (numready == -1) {
            LOG_ERROR("SDLNet_CheckSockets: " << SDLNet_GetError(), 0)
            // When this is a system error, perror may help us
            perror("SDLNet_CheckSockets");
        }
        else if (numready > 0) {
            LOG_INFO(numready << " sockets with activity!", 0)

            // Check server socket
            if (SDLNet_SocketReady(ltd->socket)) {
                TCPsocket client = SDLNet_TCP_Accept(ltd->socket);

                if (client) {
                    // Add the client socket to the socket set
                    if (SDLNet_TCP_AddSocket(set, client) < 0) {
                        LOG_ERROR("SDLNet_AddSocket: " << SDLNet_GetError(), 0)
                    }
                    else {
                        NetComputer *comp = new NetComputer(this, client);
                        clients.push_back(comp);
                        computerConnected(comp);
                        LOG_INFO(clients.size() << " client(s) connected", 0)
                    }
                }
            }

            // Check client sockets
            NetComputers::iterator i;

            for (i = clients.begin(); i != clients.end(); )
            {
                NetComputer *comp = *i;
                TCPsocket s = (*i)->getSocket();

                if (SDLNet_SocketReady(s))
                {
                    char buffer[1024];
                    int result = SDLNet_TCP_Recv(s, buffer, 1024);
                    if (result <= 0) {
                        SDLNet_TCP_DelSocket(set, s);
                        SDLNet_TCP_Close(s);
                        computerDisconnected(comp);
                        delete comp;
                        comp = NULL;
                    }
                    else {
                        // Copy the incoming data to the in buffer of this
                        // client
                        //buffer[result] = 0;
                        //LOG_INFO("Received: " << buffer << ", Length: "
                        //  << result);
                        LOG_INFO("Received length: " << result, 2);

#ifdef SCRIPT_SUPPORT
                        // This could be good if you wanted to extend the
                        // server protocol using a scripting language. This
                        // could be attained by using allowing scripts to
                        // "hook" certain messages.

                        //script->message(buffer);
#endif

                        // If the scripting subsystem didn't hook the message
                        // it will be handled by the default message handler.

                        // Convert the client IP address to string
                        // representation
                        std::string ipaddr = ip4ToString(
                                SDLNet_TCP_GetPeerAddress(s)->host);

                        // Make sure that the packet is big enough (> short)
                        if (result >= 2)
                        {
                            Packet *packet = new Packet(buffer, result);
                            MessageIn msg(packet); // (MessageIn frees packet)

                            short messageId = msg.getId();

                            if (handlers.find(messageId) != handlers.end())
                            {
                                // send message to appropriate handler
                                handlers[messageId]->receiveMessage(
                                        *comp, msg);
                            }
                            else {
                                // bad message (no registered handler)
                                LOG_ERROR("Unhandled message (" << messageId
                                          << ") received from " << ipaddr, 0);
                            }
                        }
                        else {
                            LOG_ERROR("Message too short from " << ipaddr, 0);
                        }
                    }
                }

                // Traverse to next client, possibly deleting current
                if (comp == NULL) {
                    i = clients.erase(i);
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
    LOG_INFO("A client connected!", 0)
}

void ConnectionHandler::computerDisconnected(NetComputer *comp)
{
    LOG_INFO("A client disconnected!", 0)
}

void ConnectionHandler::registerHandler(
        unsigned int msgId, MessageHandler *handler)
{
    handlers[msgId] = handler;
}

void ConnectionHandler::sendTo(tmwserv::BeingPtr beingPtr, MessageOut &msg)
{
    for (NetComputers::iterator i = clients.begin();
         i != clients.end();
         i++) {
        if ((*i)->getCharacter().get() == beingPtr.get()) {
            (*i)->send(msg.getPacket());
            break;
        }
    }
}
