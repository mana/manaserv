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
#include "game-server/postman.hpp"
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
    const std::string accountServerAddress =
        Configuration::getValue("accountServerAddress", "localhost");
    const int accountServerPort =
        Configuration::getValue("accountServerPort", DEFAULT_SERVER_PORT) + 1;

    if (!Connection::start(accountServerAddress, accountServerPort))
    {
        LOG_INFO("Unable to create a connection to an account server.");
        return false;
    }

    LOG_INFO("Connection established to the account server.");

    const std::string gameServerAddress =
        Configuration::getValue("gameServerAddress", "localhost");
    const int gameServerPort =
        Configuration::getValue("gameServerPort", DEFAULT_SERVER_PORT + 3);

    // Register with the account server and send the list of maps we handle
    MessageOut msg(GAMSG_REGISTER);
    msg.writeString(gameServerAddress);
    msg.writeShort(gameServerPort);
    MapManager::Maps const &m = MapManager::getMaps();
    for (MapManager::Maps::const_iterator i = m.begin(), i_end = m.end();
            i != i_end; ++i)
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

        case CGMSG_POST_RESPONSE:
        {
            // get the character
            Character *character = postMan->getCharacter(msg.readLong());

            // check character is still valid
            if (!character)
            {
                break;
            }

            std::string sender = msg.readString();
            std::string letter = msg.readString();

            postMan->gotPost(character, sender, letter);

        } break;

        case CGMSG_STORE_POST_RESPONSE:
        {
            // get character
            Character *character = postMan->getCharacter(msg.readLong());

            // check character is valid
            if (!character)
            {
                break;
            }

            // create message and put error inside
            MessageOut out(GPMSG_SEND_POST_RESPONSE);
            out.writeByte(msg.readByte());

            // send message to character
            gameHandler->sendTo(character, out);
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

void AccountConnection::sendPost(Character *c, MessageIn &msg)
{
    // send message to account server with id of sending player,
    // the id of receiving player, the letter contents, and attachments
    LOG_DEBUG("Sending GCMSG_STORE_POST.");
    MessageOut out(GCMSG_STORE_POST);
    out.writeLong(c->getDatabaseID());
    out.writeString(msg.readString());
    out.writeString(msg.readString());
    while (msg.getUnreadLength())
    {
        // write the item id and amount for each attachment
        out.writeLong(msg.readShort());
        out.writeLong(msg.readShort());
    }
    send(out);
}

void AccountConnection::getPost(Character *c)
{
    // let the postman know to expect some post for this character
    postMan->addCharacter(c);

    // send message to account server with id of retrieving player
    LOG_DEBUG("Sending GCMSG_REQUEST_POST");
    MessageOut out(GCMSG_REQUEST_POST);
    out.writeLong(c->getDatabaseID());
    send(out);
}

void AccountConnection::changeAccountLevel(Character *c, int level)
{
    MessageOut msg(GAMSG_CHANGE_ACCOUNT_LEVEL);
    msg.writeLong(c->getDatabaseID());
    msg.writeShort(level);
    send(msg);
}
