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

NetComputer::NetComputer(ConnectionHandler *handler, ENetPeer *peer):
    handler(handler),
    peer(peer)
{
}

void NetComputer::disconnect(const std::string &reason)
{
    // Somehow notify the netsession listener about the disconnect after
    // sending this computer a disconnect message containing the reason.
}

void NetComputer::send(const Packet *p)
{
    // Create a reliable packet.
    ENetPacket *packet = enet_packet_create(p->data, p->length,
                                            ENET_PACKET_FLAG_RELIABLE);

    // Send the packet to the peer over channel id 0.
    enet_peer_send(peer, 0, packet);
}
