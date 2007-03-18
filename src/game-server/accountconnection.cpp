/*
 *  The Mana World
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

#include "configuration.h"
#include "defines.h"
#include "game-server/accountconnection.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/character.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "utils/logger.h"
#include "utils/tokendispenser.hpp"
#include "utils/tokencollector.hpp"

bool AccountConnection::start()
{
    if (!Connection::start(config.getValue("accountServerAddress", "localhost"),
                           int(config.getValue("accountServerPort", DEFAULT_SERVER_PORT)) + 1))
    {
        return false;
    }
    LOG_INFO("Connection established to the account server.");
    MessageOut msg(GAMSG_REGISTER);
    msg.writeString(config.getValue("gameServerAddress", "localhost"));
    msg.writeShort(int(config.getValue("gameServerPort", DEFAULT_SERVER_PORT + 3)));
    MapManager::Maps const &m = mapManager->getMaps();
    for (MapManager::Maps::const_iterator i = m.begin(), i_end = m.end(); i != i_end; ++i)
    {
        msg.writeShort(i->first);
    }
    send(msg);
    return true;
}

void AccountConnection::sendCharacterData(Character *p)
{
    MessageOut msg(GAMSG_PLAYER_DATA);
    p->serialize(msg);
    send(msg);
}

void AccountConnection::processMessage(MessageIn &msg)
{
    switch (msg.getId())
    {
        case AGMSG_PLAYER_ENTER:
        {
            std::string token = msg.readString(MAGIC_TOKEN_LENGTH);
            Character *ptr = new Character(msg);
            ptr->setSpeed(150); // TODO
            gameHandler->mTokenCollector.addPendingConnect(token, ptr);
        } break;

        case AGMSG_ACTIVE_MAP:
        {
            int id = msg.readShort();
            mapManager->raiseActive(id);
        } break;

        case AGMSG_REDIRECT_RESPONSE:
        {
            int id = msg.readLong();
            std::string token = msg.readString(MAGIC_TOKEN_LENGTH);
            std::string address = msg.readString();
            int port = msg.readShort();
            gameHandler->completeServerChange(id, token, address, port);
        } break;

        default:
            LOG_WARN("Invalid message type");
            break;
    }
}

void AccountConnection::playerReconnectAccount(int id, const std::string magic_token)
{
    LOG_INFO("Send GAMSG_PLAYER_RECONNECT.");
    MessageOut msg(GAMSG_PLAYER_RECONNECT);
    msg.writeLong(id);
    msg.writeString(magic_token, MAGIC_TOKEN_LENGTH);
    send(msg);
}
