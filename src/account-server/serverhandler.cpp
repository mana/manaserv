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

#include "account-server/serverhandler.h"

#include "account-server/accountclient.h"
#include "account-server/accounthandler.h"
#include "account-server/character.h"
#include "account-server/storage.h"
#include "chat-server/post.h"
#include "common/configuration.h"
#include "common/manaserv_protocol.h"
#include "common/transaction.h"
#include "net/connectionhandler.h"
#include "net/messageout.h"
#include "net/netcomputer.h"
#include "serialize/characterdata.h"
#include "utils/logger.h"
#include "utils/tokendispenser.h"

using namespace ManaServ;

struct MapStatistics
{
  std::vector<int> players;
  unsigned short nbThings;
  unsigned short nbMonsters;
};

typedef std::map<unsigned short, MapStatistics> ServerStatistics;

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
    LOG_INFO("Game-server disconnected.");
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
    msg.writeInt32(ptr->getDatabaseID());
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
    GameServer *server = static_cast<GameServer *>(comp);

    switch (msg.getId())
    {
        case GAMSG_REGISTER:
        {
            LOG_DEBUG("GAMSG_REGISTER");
            // TODO: check the credentials of the game server
            server->address = msg.readString();
            server->port = msg.readInt16();
            const std::string password = msg.readString();

            // checks the version of the remote item database with our local copy
            unsigned int dbversion = msg.readInt32();
            LOG_INFO("Game server uses itemsdatabase with version " << dbversion);

            LOG_DEBUG("AGMSG_REGISTER_RESPONSE");
            MessageOut outMsg(AGMSG_REGISTER_RESPONSE);
            if (dbversion == storage->getItemDatabaseVersion())
            {
                LOG_DEBUG("Item databases between account server and "
                    "gameserver are in sync");
                outMsg.writeInt16(DATA_VERSION_OK);
            }
            else
            {
                LOG_DEBUG("Item database of game server has a wrong version");
                outMsg.writeInt16(DATA_VERSION_OUTDATED);
            }
            if (password == Configuration::getValue("net_password", "changeMe"))
            {
                outMsg.writeInt16(PASSWORD_OK);
                comp->send(outMsg);

                // transmit global world state variables
                std::map<std::string, std::string> variables;
                variables = storage->getAllWorldStateVars(0);
                for (std::map<std::string, std::string>::iterator i = variables.begin();
                     i != variables.end();
                     i++)
                {
                    outMsg.writeString(i->first);
                    outMsg.writeString(i->second);
                }
            }
            else
            {
                LOG_INFO("The password given by " << server->address << ':' << server->port << " was bad.");
                outMsg.writeInt16(PASSWORD_BAD);
                comp->disconnect(outMsg);
                break;
            }

            LOG_INFO("Game server " << server->address << ':' << server->port
                     << " wants to register " << (msg.getUnreadLength() / 2)
                     << " maps.");

            while (msg.getUnreadLength())
            {
                int id = msg.readInt16();
                LOG_INFO("Registering map " << id << '.');
                if (GameServer *s = getGameServerFromMap(id))
                {
                    LOG_ERROR("Server Handler: map is already registered by "
                              << s->address << ':' << s->port << '.');
                }
                else
                {
                    MessageOut outMsg(AGMSG_ACTIVE_MAP);
                    outMsg.writeInt16(id);
                    std::map<std::string, std::string> variables;
                    variables = storage->getAllWorldStateVars(id);
                    for (std::map<std::string, std::string>::iterator i = variables.begin();
                         i != variables.end();
                         i++)
                    {
                        outMsg.writeString(i->first);
                        outMsg.writeString(i->second);
                    }
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
            int id = msg.readInt32();
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
            int id = msg.readInt32();
            std::string magic_token(utils::getMagicToken());
            if (Character *ptr = storage->getCharacter(id, NULL))
            {
                int mapId = ptr->getMapId();
                if (GameServer *s = getGameServerFromMap(mapId))
                {
                    registerGameClient(s, magic_token, ptr);
                    result.writeInt16(AGMSG_REDIRECT_RESPONSE);
                    result.writeInt32(id);
                    result.writeString(magic_token, MAGIC_TOKEN_LENGTH);
                    result.writeString(s->address);
                    result.writeInt16(s->port);
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
            int id = msg.readInt32();
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

        case GAMSG_GET_VAR_CHR:
        {
            int id = msg.readInt32();
            std::string name = msg.readString();
            std::string value = storage->getQuestVar(id, name);
            result.writeInt16(AGMSG_GET_VAR_CHR_RESPONSE);
            result.writeInt32(id);
            result.writeString(name);
            result.writeString(value);
        } break;

        case GAMSG_SET_VAR_CHR:
        {
            int id = msg.readInt32();
            std::string name = msg.readString();
            std::string value = msg.readString();
            storage->setQuestVar(id, name, value);
        } break;

        case GAMSG_SET_VAR_WORLD:
        {
            std::string name = msg.readString();
            std::string value = msg.readString();
            // save the new value to the database
            storage->setWorldStateVar(name, value);
            // relay the new value to all gameservers
            for (ServerHandler::NetComputers::iterator i = clients.begin();
                i != clients.end();
                i++)
            {
                MessageOut varUpdateMessage(AGMSG_SET_VAR_WORLD);
                varUpdateMessage.writeString(name);
                varUpdateMessage.writeString(value);
                (*i)->send(varUpdateMessage);
            }
        } break;

        case GAMSG_SET_VAR_MAP:
        {
            int mapid = msg.readInt32();
            std::string name = msg.readString();
            std::string value = msg.readString();
            storage->setWorldStateVar(name, mapid, value);
        } break;

        case GAMSG_BAN_PLAYER:
        {
            int id = msg.readInt32();
            int duration = msg.readInt32();
            storage->banCharacter(id, duration);
        } break;

        case GAMSG_CHANGE_PLAYER_LEVEL:
        {
            int id = msg.readInt32();
            int level = msg.readInt16();
            storage->setPlayerLevel(id, level);
        } break;

        case GAMSG_CHANGE_ACCOUNT_LEVEL:
        {
            int id = msg.readInt32();
            int level = msg.readInt16();

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
                int mapId = msg.readInt16();
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
                m.nbThings = msg.readInt16();
                m.nbMonsters = msg.readInt16();
                int nb = msg.readInt16();
                m.players.resize(nb);
                for (int j = 0; j < nb; ++j)
                {
                    m.players[j] = msg.readInt32();
                }
            }
        } break;

        case GCMSG_REQUEST_POST:
        {
            // Retrieve the post for user
            LOG_DEBUG("GCMSG_REQUEST_POST");
            result.writeInt16(CGMSG_POST_RESPONSE);

            // get the character id
            int characterId = msg.readInt32();

            // send the character id of sender
            result.writeInt32(characterId);

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
                        result.writeInt16(items[j].itemId);
                        result.writeInt16(items[j].amount);
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
            result.writeInt16(CGMSG_STORE_POST_RESPONSE);

            // get the sender and receiver
            int senderId = msg.readInt32();
            std::string receiverName = msg.readString();

            // for sending it back
            result.writeInt32(senderId);

            // get their characters
            Character *sender = storage->getCharacter(senderId, NULL);
            Character *receiver = storage->getCharacter(receiverName);
            if (!sender || !receiver)
            {
                // Invalid character
                LOG_ERROR("Error finding character id for post");
                result.writeInt8(ERRMSG_INVALID_ARGUMENT);
                break;
            }

            // get the letter contents
            std::string contents = msg.readString();

            std::vector< std::pair<int, int> > items;
            while (msg.getUnreadLength())
            {
                items.push_back(std::pair<int, int>(msg.readInt16(), msg.readInt16()));
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

            result.writeInt8(ERRMSG_OK);
        } break;

        case GAMSG_TRANSACTION:
        {
            LOG_DEBUG("TRANSACTION");
            int id = msg.readInt32();
            int action = msg.readInt32();
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
            result.writeInt16(XXMSG_INVALID);
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
        msg.writeInt32(ptr->getDatabaseID());
        msg.writeInt32(partyId);
        s->send(msg);
    }
}

void GameServerHandler::syncDatabase(MessageIn &msg)
{
    // It is safe to perform the following updates in a transaction
    dal::PerformTransaction transaction(storage->database());

    while (msg.getUnreadLength() > 0)
    {
        int msgType = msg.readInt8();
        switch (msgType)
        {
            case SYNC_CHARACTER_POINTS:
            {
                LOG_DEBUG("received SYNC_CHARACTER_POINTS");
                int charId = msg.readInt32();
                int charPoints = msg.readInt32();
                int corrPoints = msg.readInt32();
                storage->updateCharacterPoints(charId, charPoints, corrPoints);
            } break;

            case SYNC_CHARACTER_ATTRIBUTE:
            {
                LOG_DEBUG("received SYNC_CHARACTER_ATTRIBUTE");
                int    charId = msg.readInt32();
                int    attrId = msg.readInt32();
                double base   = msg.readDouble();
                double mod    = msg.readDouble();
                storage->updateAttribute(charId, attrId, base, mod);
            } break;

            case SYNC_CHARACTER_SKILL:
            {
                LOG_DEBUG("received SYNC_CHARACTER_SKILL");
                int charId = msg.readInt32();
                int skillId = msg.readInt8();
                int skillValue = msg.readInt32();
                storage->updateExperience(charId, skillId, skillValue);
            } break;

            case SYNC_ONLINE_STATUS:
            {
                LOG_DEBUG("received SYNC_ONLINE_STATUS");
                int charId = msg.readInt32();
                bool online = (msg.readInt8() == 1);
                storage->setOnlineStatus(charId, online);
            }
        }
    }

    transaction.commit();
}
