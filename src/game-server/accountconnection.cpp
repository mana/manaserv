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
#include "player.h"
#include "game-server/accountconnection.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/mapmanager.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "utils/logger.h"

extern void registerGameClient(std::string const &, Player *);

bool AccountConnection::start()
{
    if (!Connection::start(config.getValue("accountServerAddress", "localhost"),
                           int(config.getValue("accountServerPort", DEFAULT_SERVER_PORT)) + 1))
    {
        return false;
    }
    LOG_INFO("Connection established to the account server.", 0);
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

void AccountConnection::sendPlayerData(Player *p)
{
    MessageOut msg(GAMSG_PLAYER_DATA);
    msg.writeLong(p->getDatabaseID());
    msg.writeByte(p->getGender());
    msg.writeByte(p->getHairStyle());
    msg.writeByte(p->getHairColor());
    msg.writeByte(p->getLevel());
    msg.writeShort(p->getMoney());
    for (int j = 0; j < NB_RSTAT; ++j)
        msg.writeShort(p->getRawStat(j));
    Point pos = p->getPosition();
    msg.writeShort(pos.x);
    msg.writeShort(pos.y);
    msg.writeShort(p->getMapId());
    send(msg);
}

void AccountConnection::processMessage(MessageIn &msg)
{
    switch (msg.getId())
    {
        case AGMSG_PLAYER_ENTER:
        {
            int id = msg.readLong();
            std::string name = msg.readString();
            Player *ptr = new Player(name, id);
            ptr->setGender((Gender)msg.readByte());
            ptr->setHairStyle(msg.readByte());
            ptr->setHairColor(msg.readByte());
            ptr->setLevel(msg.readByte());
            ptr->setMoney(msg.readShort());
            for (int j = 0; j < NB_RSTAT; ++j)
                ptr->setRawStat(j, msg.readShort());
            int x = msg.readShort();
            int y = msg.readShort();
            Point pos = { x, y };
            ptr->setPosition(pos);
            ptr->setMapId(msg.readShort());
            ptr->setSpeed(150); // TODO
            std::string token = msg.readString(32);
            registerGameClient(token, ptr);
        } break;

        case AGMSG_ACTIVE_MAP:
        {
            int id = msg.readShort();
            mapManager->raiseActive(id);
        } break;

        case AGMSG_REDIRECT_RESPONSE:
        {
            int id = msg.readLong();
            std::string token = msg.readString(32);
            std::string address = msg.readString();
            int port = msg.readShort();
            gameHandler->completeServerChange(id, token, address, port);
        } break;

        default:
            LOG_WARN("Invalid message type", 0);
            break;
    }
}
