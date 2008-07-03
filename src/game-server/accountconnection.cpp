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

#include "game-server/accountconnection.hpp"

#include "defines.h"
#include "common/configuration.hpp"
#include "game-server/character.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/quest.hpp"
#include "game-server/state.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "serialize/characterdata.hpp"
#include "utils/logger.h"
#include "utils/tokendispenser.hpp"
#include "utils/tokencollector.hpp"

bool AccountConnection::start()
{
    if (!Connection::start(
            Configuration::getValue("accountServerAddress", "localhost"),
            Configuration::getValue("accountServerPort", DEFAULT_SERVER_PORT) + 1))
    {
        return false;
    }
    LOG_INFO("Connection established to the account server.");
    MessageOut msg(GAMSG_REGISTER);
    msg.writeString(Configuration::getValue("gameServerAddress", "localhost"));
    msg.writeShort(Configuration::getValue("gameServerPort", DEFAULT_SERVER_PORT + 3));
    MapManager::Maps const &m = MapManager::getMaps();
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
    msg.writeLong(p->getDatabaseID());
    serializeCharacterData(*p, msg);
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
            ptr->setSpeed(250); // TODO
            gameHandler->addPendingCharacter(token, ptr);
        } break;

        case AGMSG_ACTIVE_MAP:
        {
            int id = msg.readShort();
            MapManager::raiseActive(id);
        } break;

        case AGMSG_REDIRECT_RESPONSE:
        {
            int id = msg.readLong();
            std::string token = msg.readString(MAGIC_TOKEN_LENGTH);
            std::string address = msg.readString();
            int port = msg.readShort();
            gameHandler->completeServerChange(id, token, address, port);
        } break;

        case AGMSG_GET_QUEST_RESPONSE:
        {
            int id = msg.readLong();
            std::string name = msg.readString();
            std::string value = msg.readString();
            recoveredQuestVar(id, name, value);
        } break;

        case CGMSG_CHANGED_PARTY:
        {
            // Party id, 0 for none
            int partyid = msg.readLong();
            // Character DB id
            int charid = msg.readLong();
            gameHandler->updateCharacter(charid, partyid);
        } break;

        default:
            LOG_WARN("Invalid message type");
            break;
    }
}

void AccountConnection::playerReconnectAccount(int id, std::string const &magic_token)
{
    LOG_DEBUG("Send GAMSG_PLAYER_RECONNECT.");
    MessageOut msg(GAMSG_PLAYER_RECONNECT);
    msg.writeLong(id);
    msg.writeString(magic_token, MAGIC_TOKEN_LENGTH);
    send(msg);
}

void AccountConnection::requestQuestVar(Character *ch, std::string const &name)
{
    MessageOut msg(GAMSG_GET_QUEST);
    msg.writeLong(ch->getDatabaseID());
    msg.writeString(name);
    send(msg);
}

void AccountConnection::updateQuestVar(Character *ch, std::string const &name,
                                        std::string const &value)
{
    MessageOut msg(GAMSG_SET_QUEST);
    msg.writeLong(ch->getDatabaseID());
    msg.writeString(name);
    msg.writeString(value);
    send(msg);
}

void AccountConnection::banCharacter(Character *ch, int duration)
{
    MessageOut msg(GAMSG_BAN_PLAYER);
    msg.writeLong(ch->getDatabaseID());
    msg.writeShort(duration);
    send(msg);
}

void AccountConnection::sendStatistics()
{
    MessageOut msg(GAMSG_STATISTICS);
    MapManager::Maps const &maps = MapManager::getMaps();
    for (MapManager::Maps::const_iterator i = maps.begin(),
         i_end = maps.end(); i != i_end; ++i)
    {
        MapComposite *m = i->second;
        if (!m->isActive()) continue;
        msg.writeShort(i->first);
        int nbThings = 0, nbMonsters = 0;
        typedef std::vector< Thing * > Things;
        Things const &things = m->getEverything();
        std::vector< int > players;
        for (Things::const_iterator j = things.begin(),
             j_end = things.end(); j != j_end; ++j)
        {
            Thing *t = *j;
            switch (t->getType())
            {
                case OBJECT_CHARACTER:
                    players.push_back
                        (static_cast< Character * >(t)->getDatabaseID());
                    break;
                case OBJECT_MONSTER:
                    ++nbMonsters;
                    break;
                default:
                    ++nbThings;
            }
        }
        msg.writeShort(nbThings);
        msg.writeShort(nbMonsters);
        msg.writeShort(players.size());
        for (std::vector< int >::const_iterator j = players.begin(),
             j_end = players.end(); j != j_end; ++j)
        {
            msg.writeLong(*j);
        }
    }
    send(msg);
}

