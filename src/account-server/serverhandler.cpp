/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <sstream>
#include <list>

#include "account-server/serverhandler.hpp"

#include "account-server/accountclient.hpp"
#include "account-server/accounthandler.hpp"
#include "account-server/character.hpp"
#include "account-server/storage.hpp"
#include "chat-server/post.hpp"
#include "common/transaction.hpp"
#include "common/configuration.hpp"
#include "net/connectionhandler.hpp"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "serialize/characterdata.hpp"
#include "utils/logger.h"
#include "utils/tokendispenser.hpp"
#include "protocol.h"

struct MapStatistics
{
  std::vector< int > players;
  unsigned short nbThings;
  unsigned short nbMonsters;
};

typedef std::map< unsigned short, MapStatistics > ServerStatistics;

/**
 * Stores address, maps, and statistics, of a connected game server.
 */
struct GameServer: NetComputer
{
    GameServer(ENetPeer *peer): NetComputer(peer), port(0) {}

    std::string address;
    NetComputer *server;
    ServerStatistics maps;
    short port;
};

static GameServer *getGameServerFromMap(int);

/**
 * Manages communications with all the game servers.
 */
class ServerHandler: public ConnectionHandler
{
    friend GameServer *getGameServerFromMap(int);
    friend void GameServerHandler::dumpStatistics(std::ostream &);

    protected:
        /**
         * Processes server messages.
         */
        void processMessage(NetComputer *computer, MessageIn &message);

        /**
         * Called when a game server connects. Initializes a simple NetComputer
         * as these connections are stateless.
         */
        NetComputer *computerConnected(ENetPeer *peer);

        /**
         * Called when a game server disconnects.
         */
        void computerDisconnected(NetComputer *comp);
};

static ServerHandler *serverHandler;

bool GameServerHandler::initialize(int port, const std::string &host)
{
    serverHandler = new ServerHandler;
    LOG_INFO("Game server handler started:");
    return serverHandler->startListen(port, host);
}

void GameServerHandler::deinitialize()
{
    serverHandler->stopListen();
    delete serverHandler;
}

void GameServerHandler::process()
{
    serverHandler->process(50);
}

NetComputer *ServerHandler::computerConnected(ENetPeer *peer)
{
    return new GameServer(peer);
}

void ServerHandler::computerDisconnected(NetComputer *comp)
{
    delete comp;
}

static GameServer *getGameServerFromMap(int mapId)
{
    for (ServerHandler::NetComputers::const_iterator
         i = serverHandler->clients.begin(),
         i_end = serverHandler->clients.end(); i != i_end; ++i)
    {
        GameServer *server = static_cast< GameServer * >(*i);
        ServerStatistics::const_iterator i = server->maps.find(mapId);
        if (i == server->maps.end()) continue;
        return server;
    }
    return NULL;
}

bool GameServerHandler::getGameServerFromMap(int mapId,
                                             std::string &address,
                                             int &port)
{
    if (GameServer *s = ::getGameServerFromMap(mapId))
    {
        address = s->address;
        port = s->port;
        return true;
    }
    return false;
}

static void registerGameClient(GameServer *s, const std::string &token,
                               Character *ptr)
{
    MessageOut msg(AGMSG_PLAYER_ENTER);
    msg.writeString(token, MAGIC_TOKEN_LENGTH);
    msg.writeLong(ptr->getDatabaseID());
    msg.writeString(ptr->getName());
    serializeCharacterData(*ptr, msg);
    s->send(msg);
}

void GameServerHandler::registerClient(const std::string &token,
                                       Character *ptr)
{
    GameServer *s = ::getGameServerFromMap(ptr->getMapId());
    assert(s);
    registerGameClient(s, token, ptr);
}

void ServerHandler::processMessage(NetComputer *comp, MessageIn &msg)
{
    MessageOut result;
    GameServer *server = static_cast< GameServer * >(comp);

    switch (msg.getId())
    {
        case GAMSG_REGISTER:
        {
            LOG_DEBUG("GAMSG_REGISTER");
            // TODO: check the credentials of the game server
            server->address = msg.readString();
            server->port = msg.readShort();
            const std::string password = msg.readString();

            // checks the version of the remote item database with our local copy
            unsigned int dbversion = msg.readLong();
            LOG_INFO("Game server uses itemsdatabase with version " << dbversion);

            LOG_DEBUG("AGMSG_REGISTER_RESPONSE");
            MessageOut outMsg(AGMSG_REGISTER_RESPONSE);
            if (dbversion == storage->getItemDatabaseVersion())
            {
                LOG_DEBUG("Item databases between account server and "
                    "gameserver are in sync");
                outMsg.writeShort(DATA_VERSION_OK);
            }
            else
            {
                LOG_DEBUG("Item database of game server has a wrong version");
                outMsg.writeShort(DATA_VERSION_OUTDATED);
            }
            if (password == Configuration::getValue("net_password", "P@s$w0rd"))
            {
                outMsg.writeShort(PASSWORD_OK);
                comp->send(outMsg);
            }
            else
            {
                LOG_INFO("The password given by " << server->address << ':' << server->port << " was bad.");
                outMsg.writeShort(PASSWORD_BAD);
                comp->disconnect(outMsg);
                break;
            }

            LOG_INFO("Game server " << server->address << ':' << server->port
                     << " wants to register " << (msg.getUnreadLength() / 2)
                     << " maps.");

            while (msg.getUnreadLength())
            {
                int id = msg.readShort();
                LOG_INFO("Registering map " << id << '.');
                if (GameServer *s = getGameServerFromMap(id))
                {
                    LOG_ERROR("Server Handler: map is already registered by "
                              << s->address << ':' << s->port << '.');
                }
                else
                {
                    MessageOut outMsg(AGMSG_ACTIVE_MAP);
                    outMsg.writeShort(id);
                    comp->send(outMsg);
                    MapStatistics &m = server->maps[id];
                    m.nbThings = 0;
                    m.nbMonsters = 0;
                }
            }
        } break;

        case GAMSG_PLAYER_DATA:
        {
            LOG_DEBUG("GAMSG_PLAYER_DATA");
            int id = msg.readLong();
            if (Character *ptr = storage->getCharacter(id, NULL))
            {
                deserializeCharacterData(*ptr, msg);
                if (!storage->updateCharacter(ptr))
                {
                    LOG_ERROR("Failed to update character "
                              << id << '.');
                }
                delete ptr;
            }
            else
            {
                LOG_ERROR("Received data for non-existing character "
                          << id << '.');
            }
        } break;

        case GAMSG_PLAYER_SYNC:
        {
            LOG_DEBUG("GAMSG_PLAYER_SYNC");
            GameServerHandler::syncDatabase(msg);
        } break;

        case GAMSG_REDIRECT:
        {
            LOG_DEBUG("GAMSG_REDIRECT");
            int id = msg.readLong();
            std::string magic_token(utils::getMagicToken());
            if (Character *ptr = storage->getCharacter(id, NULL))
            {
                int mapId = ptr->getMapId();
                if (GameServer *s = getGameServerFromMap(mapId))
                {
                    registerGameClient(s, magic_token, ptr);
                    result.writeShort(AGMSG_REDIRECT_RESPONSE);
                    result.writeLong(id);
                    result.writeString(magic_token, MAGIC_TOKEN_LENGTH);
                    result.writeString(s->address);
                    result.writeShort(s->port);
                }
                else
                {
                    LOG_ERROR("Server Change: No game server for map " <<
                              mapId << '.');
                }
                delete ptr;
            }
            else
            {
                LOG_ERROR("Received data for non-existing character "
                          << id << '.');
            }
        } break;

        case GAMSG_PLAYER_RECONNECT:
        {
            LOG_DEBUG("GAMSG_PLAYER_RECONNECT");
            int id = msg.readLong();
            std::string magic_token = msg.readString(MAGIC_TOKEN_LENGTH);

            if (Character *ptr = storage->getCharacter(id, NULL))
            {
                int accountID = ptr->getAccountID();
                AccountClientHandler::prepareReconnect(magic_token, accountID);
                delete ptr;
            }
            else
            {
                LOG_ERROR("Received data for non-existing character "
                          << id << '.');
            }
        } break;

        case GAMSG_GET_QUEST:
        {
            int id = msg.readLong();
            std::string name = msg.readString();
            std::string value = storage->getQuestVar(id, name);
            result.writeShort(AGMSG_GET_QUEST_RESPONSE);
            result.writeLong(id);
            result.writeString(name);
            result.writeString(value);
        } break;

        case GAMSG_SET_QUEST:
        {
            int id = msg.readLong();
            std::string name = msg.readString();
            std::string value = msg.readString();
            storage->setQuestVar(id, name, value);
        } break;

        case GAMSG_BAN_PLAYER:
        {
            int id = msg.readLong();
            int duration = msg.readShort();
            storage->banCharacter(id, duration);
        } break;

        case GAMSG_CHANGE_PLAYER_LEVEL:
        {
            int id = msg.readLong();
            int level = msg.readShort();
            storage->setPlayerLevel(id, level);
        } break;

        case GAMSG_CHANGE_ACCOUNT_LEVEL:
        {
            int id = msg.readLong();
            int level = msg.readShort();

            // get the character so we can get the account id
            Character *c = storage->getCharacter(id, NULL);
            if (c)
            {
                storage->setAccountLevel(c->getAccountID(), level);
            }
        } break;

        case GAMSG_STATISTICS:
        {
            while (msg.getUnreadLength())
            {
                int mapId = msg.readShort();
                ServerStatistics::iterator i = server->maps.find(mapId);
                if (i == server->maps.end())
                {
                    LOG_ERROR("Server " << server->address << ':'
                              << server->port << " should not be sending stati"
                              "stics for map " << mapId << '.');
                    // Skip remaining data.
                    break;
                }
                MapStatistics &m = i->second;
                m.nbThings = msg.readShort();
                m.nbMonsters = msg.readShort();
                int nb = msg.readShort();
                m.players.resize(nb);
                for (int j = 0; j < nb; ++j)
                {
                    m.players[j] = msg.readLong();
                }
            }
        } break;

        case GCMSG_REQUEST_POST:
        {
            // Retrieve the post for user
            LOG_DEBUG("GCMSG_REQUEST_POST");
            result.writeShort(CGMSG_POST_RESPONSE);

            // get the character id
            int characterId = msg.readLong();

            // send the character id of sender
            result.writeLong(characterId);

            // get the character based on the id
            Character *ptr = storage->getCharacter(characterId, NULL);
            if (!ptr)
            {
                // Invalid character
                LOG_ERROR("Error finding character id for post");
                break;
            }

            // get the post for that character
            Post *post = postalManager->getPost(ptr);

            // send the post if valid
            if (post)
            {
                for (unsigned int i = 0; i < post->getNumberOfLetters(); ++i)
                {
                    // get each letter, send the sender's name,
                    // the contents and any attachments
                    Letter *letter = post->getLetter(i);
                    result.writeString(letter->getSender()->getName());
                    result.writeString(letter->getContents());
                    std::vector<InventoryItem> items = letter->getAttachments();
                    for (unsigned int j = 0; j < items.size(); ++j)
                    {
                        result.writeShort(items[j].itemId);
                        result.writeShort(items[j].amount);
                    }
                }

                // clean up
                postalManager->clearPost(ptr);
            }

        } break;

        case GCMSG_STORE_POST:
        {
            // Store the letter for the user
            LOG_DEBUG("GCMSG_STORE_POST");
            result.writeShort(CGMSG_STORE_POST_RESPONSE);

            // get the sender and receiver
            int senderId = msg.readLong();
            std::string receiverName = msg.readString();

            // for sending it back
            result.writeLong(senderId);

            // get their characters
            Character *sender = storage->getCharacter(senderId, NULL);
            Character *receiver = storage->getCharacter(receiverName);
            if (!sender || !receiver)
            {
                // Invalid character
                LOG_ERROR("Error finding character id for post");
                result.writeByte(ERRMSG_INVALID_ARGUMENT);
                break;
            }

            // get the letter contents
            std::string contents = msg.readString();

            std::vector< std::pair<int, int> > items;
            while (msg.getUnreadLength())
            {
                items.push_back(std::pair<int, int>(msg.readShort(), msg.readShort()));
            }

            // save the letter
            LOG_DEBUG("Creating letter");
            Letter *letter = new Letter(0, sender, receiver);
            letter->addText(contents);
            for (unsigned int i = 0; i < items.size(); ++i)
            {
                InventoryItem item;
                item.itemId = items[i].first;
                item.amount = items[i].second;
                letter->addAttachment(item);
            }
            postalManager->addLetter(letter);

            result.writeByte(ERRMSG_OK);
        } break;

        case GAMSG_TRANSACTION:
        {
            LOG_DEBUG("TRANSACTION");
            int id = msg.readLong();
            int action = msg.readLong();
            std::string message = msg.readString();

            Transaction trans;
            trans.mCharacterId = id;
            trans.mAction = action;
            trans.mMessage = message;
            storage->addTransaction(trans);
        } break;

        default:
            LOG_WARN("ServerHandler::processMessage, Invalid message type: "
                     << msg.getId());
            result.writeShort(XXMSG_INVALID);
            break;
    }

    // return result
    if (result.getLength() > 0)
        comp->send(result);
}

void GameServerHandler::dumpStatistics(std::ostream &os)
{
    for (ServerHandler::NetComputers::const_iterator
         i = serverHandler->clients.begin(),
         i_end = serverHandler->clients.end(); i != i_end; ++i)
    {
        GameServer *server = static_cast< GameServer * >(*i);
        if (!server->port)
            continue;

        os << "<gameserver address=\"" << server->address << "\" port=\""
           << server->port << "\">\n";

        for (ServerStatistics::const_iterator j = server->maps.begin(),
             j_end = server->maps.end(); j != j_end; ++j)
        {
            const MapStatistics &m = j->second;
            os << "<map id=\"" << j->first << "\" nb_things=\"" << m.nbThings
               << "\" nb_monsters=\"" << m.nbMonsters << "\">\n";
            for (std::vector< int >::const_iterator k = m.players.begin(),
                 k_end = m.players.end(); k != k_end; ++k)
            {
                os << "<character id=\"" << *k << "\"/>\n";
            }
            os << "</map>\n";
        }
        os << "</gameserver>\n";
    }
}

void GameServerHandler::sendPartyChange(Character *ptr, int partyId)
{
    GameServer *s = ::getGameServerFromMap(ptr->getMapId());
    if (s)
    {
        MessageOut msg(CGMSG_CHANGED_PARTY);
        msg.writeLong(ptr->getDatabaseID());
        msg.writeLong(partyId);
        s->send(msg);
    }
}

void GameServerHandler::syncDatabase(MessageIn &msg)
{
    int msgType = msg.readByte();
    while (msgType != SYNC_END_OF_BUFFER)
    {
        switch (msgType)
        {
            case SYNC_CHARACTER_POINTS:
            {
                LOG_DEBUG("received SYNC_CHARACTER_POINTS");
                int CharId = msg.readLong();
                int CharPoints = msg.readLong();
                int CorrPoints = msg.readLong();
                int AttribId = msg.readByte();
                int AttribValue = msg.readLong();
                storage->updateCharacterPoints(CharId, CharPoints, CorrPoints,
                                               AttribId, AttribValue);
            } break;

            case SYNC_CHARACTER_SKILL:
            {
                LOG_DEBUG("received SYNC_CHARACTER_SKILL");
                int CharId = msg.readLong();
                int SkillId = msg.readByte();
                int SkillValue = msg.readLong();
                storage->updateExperience(CharId, SkillId, SkillValue);
            } break;

            case SYNC_ONLINE_STATUS:
            {
                LOG_DEBUG("received SYNC_ONLINE_STATUS");
                int CharId = msg.readLong();
                bool online;
                msg.readByte() == 0x00 ? online = false : online = true;
                storage->setOnlineStatus(CharId, online);
            }
        }

        // read next message type from buffer
        msgType = msg.readByte();
    }
}
