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
#include "account-server/storage.hpp"
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
    unsigned mapId = ptr->getMap();
    MessageOut msg(AGMSG_PLAYER_ENTER);
    msg.writeLong(ptr->getDatabaseID());
    msg.writeString(ptr->getName());
    msg.writeString(token, 32);
    ptr->serialize(msg);
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
                int id = msg.readShort();
                LOG_INFO("Registering map " << id << '.', 0);
                if (servers.insert(std::make_pair(id, s)).second)
                {
                    MessageOut outMsg(AGMSG_ACTIVE_MAP);
                    outMsg.writeShort(id);
                    comp->send(outMsg);
                }
                else
                {
                    LOG_ERROR("Server Handler: map is already registered.", 0);
                }
            }
        } break;

        case GAMSG_PLAYER_DATA:
        {
            int id = msg.readLong();
            Storage &store = Storage::instance("tmw");
            PlayerPtr ptr = store.getCharacter(id);
            ptr->deserialize(msg);
        } break;

        case GAMSG_REDIRECT:
        {
            int id = msg.readLong();
            std::string magic_token(32, ' ');
            for (int i = 0; i < 32; ++i)
            {
                magic_token[i] = 1 + (int)(127 * (rand() / (RAND_MAX + 1.0)));
            }
            Storage &store = Storage::instance("tmw");
            PlayerPtr ptr = store.getCharacter(id);
            std::string address;
            short port;
            if (serverHandler->getGameServerFromMap(ptr->getMap(), address, port))
            {
                registerGameClient(magic_token, ptr);
                result.writeShort(AGMSG_REDIRECT_RESPONSE);
                result.writeLong(ptr->getDatabaseID());
                result.writeString(magic_token, 32);
                result.writeString(address);
                result.writeShort(port);
            }
            else
            {
                LOG_ERROR("Server Change: No game server for the map.", 0);
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
