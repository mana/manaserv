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

#include <iosfwd>
#include <queue>
#include <enet/enet.h>

#include "defines.h"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "utils/logger.h"
#include "utils/processorutils.hpp"

NetComputer::NetComputer(ENetPeer *peer):
    mPeer(peer)
{
}

bool
NetComputer::isConnected()
{
    return (mPeer->state == ENET_PEER_STATE_CONNECTED);
}

void
NetComputer::disconnect(const MessageOut &msg)
{
    if (isConnected())
    {
        /* ChannelID 0xFF is the channel used by enet_peer_disconnect.
         * If a reliable packet is send over this channel ENet guaranties
         * that the message is recieved before the disconnect request.
         */
        send(msg, ENET_PACKET_FLAG_RELIABLE, 0xFF);

        /* ENet generates a disconnect event
         * (notifying the connection handler).
         */
        enet_peer_disconnect(mPeer, 0);
    }
}

void
NetComputer::send(const MessageOut &msg, bool reliable,
                  unsigned int channel)
{
    LOG_DEBUG("Sending packet of length " << msg.getLength() << " to " << *this);

    ENetPacket *packet;
    packet = enet_packet_create(msg.getData(),
                                msg.getLength(),
                                reliable ? ENET_PACKET_FLAG_RELIABLE : 0);

    if (packet)
    {
        enet_peer_send(mPeer, channel, packet);
    }
    else
    {
        LOG_ERROR("Failure to create packet!");
    }
}

std::ostream&
operator <<(std::ostream &os, const NetComputer &comp)
{
    // address.host contains the ip-address in network-byte-order
    if (utils::processor::isLittleEndian)
        os << ( comp.mPeer->address.host & 0x000000ff)        << "."
           << ((comp.mPeer->address.host & 0x0000ff00) >> 8)  << "."
           << ((comp.mPeer->address.host & 0x00ff0000) >> 16) << "."
           << ((comp.mPeer->address.host & 0xff000000) >> 24);
    else
    // big-endian
        os << ((comp.mPeer->address.host & 0xff000000) >> 24)  << "."
           << ((comp.mPeer->address.host & 0x00ff0000) >> 16)  << "."
           << ((comp.mPeer->address.host & 0x0000ff00) >> 8)   << "."
           << ((comp.mPeer->address.host & 0x000000ff) >> 0);

    return os;
}
