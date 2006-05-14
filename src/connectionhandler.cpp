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

#include "connectionhandler.h"

#include "chatchannelmanager.h"
#include "messagehandler.h"
#include "messagein.h"
#include "messageout.h"
#include "netcomputer.h"
#include "netsession.h"
#include "packet.h"

#include "utils/logger.h"

#ifdef SCRIPT_SUPPORT
#include "script.h"
#endif

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
    while (ltd->running)
    {
        ENetEvent event;

        // Wait up to 1000 milliseconds for an event.
        while (enet_host_service(ltd->host, &event, 1000) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                {
                    LOG_INFO("A new client connected from " <<
                             ip4ToString(event.peer->address.host) << ":" <<
                             event.peer->address.port, 0);
                    NetComputer *comp = new NetComputer(this, event.peer);
                    clients.push_back(comp);
                    computerConnected(comp);
                    /*LOG_INFO(ltd->host->peerCount <<
                             " client(s) connected", 0);*/

                    // Store any relevant client information here.
                    event.peer->data = (void *)comp;
                } break;

                case ENET_EVENT_TYPE_RECEIVE:
                {
                    LOG_INFO("A packet of length " << event.packet->dataLength
                             << " was received from " << event.peer->address.host,
                             2);

                    NetComputer *comp = (NetComputer *)event.peer->data;

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
                    std::string ipaddr = ip4ToString(event.peer->address.host);

                    // Make sure that the packet is big enough (> short)
                    if (event.packet->dataLength >= 2)
                    {
                        Packet *packet = new Packet((char *)event.packet->data,
                                                    event.packet->dataLength);
                        MessageIn msg(packet); // (MessageIn frees packet)

                        short messageId = msg.getId();

                        if (handlers.find(messageId) != handlers.end())
                        {
                            // send message to appropriate handler
                            handlers[messageId]->receiveMessage(*comp, msg);
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

                    /* Clean up the packet now that we're done using it. */
                    enet_packet_destroy(event.packet);
                } break;

                case ENET_EVENT_TYPE_DISCONNECT:
                {
                    NetComputer *comp = (NetComputer *)event.peer->data;
                    /*LOG_INFO(event.peer->address.host
                             << " disconected.", 0);*/
                    // Reset the peer's client information.
                    computerDisconnected(comp);
                    delete comp;
                    event.peer->data = NULL;
                } break;

                default: break;
            }
        }
    }

    // - Disconnect all clients (close sockets)

    // TODO: probably there's a better way.
    ENetPeer *currentPeer;

    for (currentPeer = ltd->host->peers;
         currentPeer < &ltd->host->peers[ltd->host->peerCount];
         ++currentPeer)
    {
       if (currentPeer->state == ENET_PEER_STATE_CONNECTED)
       {
            enet_peer_disconnect(currentPeer, 0);
            enet_host_flush(ltd->host);
            enet_peer_reset(currentPeer);
       }
    }
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

void ConnectionHandler::sendTo(std::string name, MessageOut &msg)
{
    for (NetComputers::iterator i = clients.begin();
         i != clients.end();
         i++) {
        if ((*i)->getCharacter().get()->getName() == name) {
            (*i)->send(msg.getPacket());
            break;
        }
    }
}

void ConnectionHandler::sendToEveryone(MessageOut &msg)
{
    for (NetComputers::iterator i = clients.begin();
         i != clients.end();
         i++)
    {
            (*i)->send(msg.getPacket());
            break;
    }
}

void ConnectionHandler::sendAround(tmwserv::BeingPtr beingPtr, MessageOut &msg)
{
    unsigned speakerMapId = beingPtr->getMapId();
    std::pair<unsigned, unsigned> speakerXY = beingPtr->getXY();
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end;
         ++i) {
        // See if the other being is near enough, then send the message
        tmwserv::Being const *listener = (*i)->getCharacter().get();
        if (listener->getMapId() != speakerMapId) continue;
        std::pair<unsigned, unsigned> listenerXY = listener->getXY();
        if (abs(listenerXY.first  - speakerXY.first ) > (int)AROUND_AREA_IN_TILES) continue;
        if (abs(listenerXY.second - speakerXY.second) > (int)AROUND_AREA_IN_TILES) continue;
        (*i)->send(msg.getPacket());
    }
}

void ConnectionHandler::sendInChannel(short channelId, MessageOut &msg)
{
    for (NetComputers::iterator i = clients.begin(); i != clients.end();i++)
    {
        const std::vector<tmwserv::BeingPtr> beingList =
            chatChannelManager->getUserListInChannel(channelId);
        // If the being is in the channel, send it
        for (std::vector<tmwserv::BeingPtr>::const_iterator j = beingList.begin();
             j != beingList.end(); j++)
        {
            if ((*i)->getCharacter().get() == (*j).get() )
            {
                (*i)->send(msg.getPacket());
            }
        }
    }
}

unsigned int ConnectionHandler::getClientNumber()
{
    return clients.size();
}

void ConnectionHandler::makeUsersLeaveChannel(const short channelId)
{
    MessageOut result;
    result.writeShort(SMSG_QUIT_CHANNEL_RESPONSE);
    result.writeByte(CHATCNL_OUT_OK);

    const std::vector<tmwserv::BeingPtr> beingList =
            chatChannelManager->getUserListInChannel(channelId);
    for (NetComputers::iterator i = clients.begin(); i != clients.end();i++)
    {
        // If the being is in the channel, send it the 'leave now' packet
        for (std::vector<tmwserv::BeingPtr>::const_iterator j = beingList.begin();
             j != beingList.end(); j++)
        {
            if ((*i)->getCharacter().get() == (*j).get() )
            {
                (*i)->send(result.getPacket());
            }
        }
    }
}

void ConnectionHandler::warnUsersAboutPlayerEventInChat(const short channelId,
                                                        const std::string& userName,
                                                        const char eventId)
{
    MessageOut result;
    result.writeShort(SMSG_UPDATE_CHANNEL_RESPONSE);
    result.writeByte(eventId);
    result.writeString(userName);

    const std::vector<tmwserv::BeingPtr> beingList =
            chatChannelManager->getUserListInChannel(channelId);
    for (NetComputers::iterator i = clients.begin(); i != clients.end();i++)
    {
        // If the being is in the channel, send it the 'eventId' packet
        for (std::vector<tmwserv::BeingPtr>::const_iterator j = beingList.begin();
             j != beingList.end(); j++)
        {
            if ((*i)->getCharacter().get() == (*j).get() )
            {
                (*i)->send(result.getPacket());
            }
        }
    }
}
