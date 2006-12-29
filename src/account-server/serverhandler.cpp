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

#include <cassert>
#include <sstream>

#include "account-server/serverhandler.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "utils/logger.h"

bool ServerHandler::startListen(enet_uint16 port)
{
    LOG_INFO("Server handler started:", 0);
    return ConnectionHandler::startListen(port);
}

NetComputer *ServerHandler::computerConnected(ENetPeer *peer)
{
    return new NetComputer(peer);
}

void ServerHandler::computerDisconnected(NetComputer *comp)
{
    Servers::iterator i = servers.begin();
    while (i != servers.end())
    {
        if (i->second.server == comp)
        {
            LOG_INFO("Unregistering map " << i->first << '.', 0);
            servers.erase(i++);
        }
        else
        {
            ++i;
        }
    }
    delete comp;
}

bool ServerHandler::getGameServerFromMap(unsigned mapId, std::string &address, short &port)
{
    Servers::const_iterator i = servers.find(mapId);
    if (i == servers.end()) return false;
    address = i->second.address;
    port = i->second.port;
    return true;
}

void ServerHandler::registerGameClient(std::string const &token, PlayerPtr ptr)
{
    unsigned mapId = ptr->getMapId();
    MessageOut msg(AGMSG_PLAYER_ENTER);
    msg.writeLong(ptr->getDatabaseID());
    msg.writeString(ptr->getName());
    msg.writeByte(ptr->getGender());
    msg.writeByte(ptr->getHairStyle());
    msg.writeByte(ptr->getHairColor());
    msg.writeByte(ptr->getLevel());
    msg.writeShort(ptr->getMoney());
    for (int j = 0; j < NB_RSTAT; ++j)
        msg.writeShort(ptr->getRawStat(j));
    Point pos = ptr->getPosition();
    msg.writeShort(pos.x);
    msg.writeShort(pos.y);
    msg.writeShort(mapId);
    msg.writeString(token, 32);
    Servers::const_iterator i = servers.find(mapId);
    assert(i != servers.end());
    i->second.server->send(msg);
}

void ServerHandler::processMessage(NetComputer *comp, MessageIn &msg)
{
    MessageOut result;

    switch (msg.getId())
    {
        case GAMSG_REGISTER:
        {
            // TODO: check the credentials of the game server
            std::string address = msg.readString();
            int port = msg.readShort();
            Server s = { address, port, comp };
            LOG_INFO("Game server " << address << ':' << port
                     << " wants to register " << (msg.getUnreadLength() / 2)
                     << " maps.", 0);
            
            while (msg.getUnreadLength())
            {
                unsigned id = msg.readShort();
                LOG_INFO("Registering map " << id << '.', 0);
                if (!servers.insert(std::make_pair(id, s)).second)
                {
                    LOG_ERROR("Server Handler: map is already registered.", 0);
                }
            }
        } break;

        default:
            LOG_WARN("Invalid message type.", 0);
            result.writeShort(XXMSG_INVALID);
            break;
    }

    // return result
    if (result.getLength() > 0)
        comp->send(result);
}
