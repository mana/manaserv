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

#include "netcomputer.h"

#include "chatchannelmanager.h"
#include "connectionhandler.h"
#include "packet.h"
#include "state.h"

#include "utils/logger.h"

NetComputer::NetComputer(ConnectionHandler *handler, ENetPeer *peer):
    mHandler(handler),
    mPeer(peer)
{
}

void NetComputer::disconnect(const std::string &reason)
{
    // TODO: Send a disconnect message containing the reason, and somehow only
    // TODO: really disconnect the client after waiting for the client to get
    // TODO: the message (or likely got it).

    // ENet should generate a disconnect event (notifying the connection
    // handler)
    enet_peer_disconnect(mPeer, 0);
}

void NetComputer::send(const Packet *p)
{
    LOG_INFO("Sending packet of length " << p->length, 2);

    // Create a reliable packet.
    ENetPacket *packet = enet_packet_create(p->data, p->length,
                                            ENET_PACKET_FLAG_RELIABLE);

    // Send the packet to the peer over channel id 0.
    enet_peer_send(mPeer, 0, packet);
}
