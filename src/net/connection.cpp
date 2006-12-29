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
 *
 *  $Id$
 */

#include "net/connection.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "utils/logger.h"

bool Connection::start(std::string const &address, int port)
{
    ENetAddress enetAddress;
    enet_address_set_host(&enetAddress, address.c_str());
    enetAddress.port = port;

    mLocal = enet_host_create(NULL    /* create a client host */,
                              1       /* allow one outgoing connection */,
                              0       /* assume any amount of incoming bandwidth */,
                              0       /* assume any amount of outgoing bandwidth */);

    if (!mLocal) return false;

    // Initiate the connection, allocating channel 0.
    mRemote = enet_host_connect(mLocal, &enetAddress, 1);

    ENetEvent event;
    if (enet_host_service(mLocal, &event, 10000) <= 0 ||
        event.type != ENET_EVENT_TYPE_CONNECT)
    {
        stop();
        return false;
    }
    return mRemote;
}

void Connection::stop()
{
    enet_peer_disconnect(mRemote, 0);
    enet_host_flush(mLocal);
    enet_peer_reset(mRemote);
    enet_host_destroy(mLocal);
    mRemote = NULL;
}

bool Connection::isConnected() const
{
    return mRemote && mRemote->state == ENET_PEER_STATE_CONNECTED;
}

void Connection::send(MessageOut const &msg, bool reliable, unsigned channel)
{
    ENetPacket *packet;
    packet = enet_packet_create(msg.getData(),
                                msg.getLength(),
                                reliable ? ENET_PACKET_FLAG_RELIABLE : 0);

    if (packet)
    {
        enet_peer_send(mRemote, channel, packet);
    }
    else
    {
        LOG_WARN("Failure to create packet!", 0);
    }
}

void Connection::process()
{
    ENetEvent event;
    // Process Enet events and do not block.
    while (enet_host_service(mLocal, &event, 0) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_RECEIVE:
                if (event.packet->dataLength >= 2) {
                    MessageIn msg((char *)event.packet->data,
                                  event.packet->dataLength);
                    processMessage(msg);
                } else {
                    LOG_ERROR("Message too short.", 0);
                }
                // Clean up the packet now that we are done using it.
                enet_packet_destroy(event.packet);
                break;

            default:
                break;
        }
    }
}
