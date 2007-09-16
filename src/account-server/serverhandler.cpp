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

#include "account-server/accountclient.hpp"
#include "account-server/accounthandler.hpp"
#include "account-server/character.hpp"
#include "account-server/dalstorage.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "serialize/characterdata.hpp"
#include "utils/logger.h"
#include "utils/tokendispenser.hpp"
#include "utils/tokencollector.hpp"

struct MapStatistics
{
  std::vector< int > players;
  unsigned short nbThings;
  unsigned short nbMonsters;
};

typedef std::map< unsigned short, MapStatistics > ServerStatistics;

struct GameServer: NetComputer
{
    GameServer(ENetPeer *peer): NetComputer(peer), port(0) {}

    std::string address;
    NetComputer *server;
    ServerStatistics maps;
    short port;
};

bool ServerHandler::startListen(enet_uint16 port)
{
    LOG_INFO("Game server handler started:");
    return ConnectionHandler::startListen(port);
}

NetComputer *ServerHandler::computerConnected(ENetPeer *peer)
{
    return new GameServer(peer);
}

void ServerHandler::computerDisconnected(NetComputer *comp)
{
    delete comp;
}

GameServer *ServerHandler::getGameServerFromMap(int mapId) const
{
    for (NetComputers::const_iterator i = clients.begin(),
         i_end = clients.end(); i != i_end; ++i)
    {
        GameServer *server = static_cast< GameServer * >(*i);
        ServerStatistics::const_iterator i = server->maps.find(mapId);
        if (i == server->maps.end()) continue;
        return server;
    }
    return NULL;
}

bool ServerHandler::getGameServerFromMap(int mapId, std::string &address,
                                         int &port) const
{
    if (GameServer *s = getGameServerFromMap(mapId))
    {
        address = s->address;
        port = s->port;
        return true;
    }
    return false;
}

void ServerHandler::registerGameClient(std::string const &token, Character *ptr)
{
    int mapId = ptr->getMapId();

    MessageOut msg(AGMSG_PLAYER_ENTER);
    msg.writeString(token, MAGIC_TOKEN_LENGTH);
    msg.writeLong(ptr->getDatabaseID());
    msg.writeString(ptr->getName());
    serializeCharacterData(*ptr, msg);

    GameServer *s = getGameServerFromMap(mapId);
    assert(s);
    s->send(msg);
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
                    registerGameClient(magic_token, ptr);
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
                accountHandler->
                    mTokenCollector.addPendingConnect(magic_token, accountID);
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

#if 0
        case GAMSG_GUILD_CREATE:
        {
            LOG_DEBUG("GAMSG_GUILD_CREATE");

            result.writeShort(AGMSG_GUILD_CREATE_RESPONSE);
            // Check if the guild name is taken already
            int playerId = msg.readLong();
            std::string guildName = msg.readString();
            if (guildManager->findByName(guildName) != NULL)
            {
                result.writeByte(ERRMSG_ALREADY_TAKEN);
                break;
            }
            result.writeByte(ERRMSG_OK);

            Storage &store = Storage::instance("tmw");
            CharacterPtr ptr = store.getCharacter(playerId);

            // Add guild to character data.
            ptr->addGuild(guildName);

            // Who to send data to at the other end
            result.writeLong(playerId);

            short guildId = guildManager->createGuild(guildName, ptr.get());
            result.writeShort(guildId);
            result.writeString(guildName);
            result.writeShort(1);
            enterChannel(guildName, ptr.get());
        } break;

        case GAMSG_GUILD_INVITE:
        {
            // Add Inviting member to guild here
            LOG_DEBUG("Received msg ... GAMSG_GUILD_INVITE");
            result.writeShort(AGMSG_GUILD_INVITE_RESPONSE);
            // Check if user can invite users
            int playerId = msg.readLong();
            short id = msg.readShort();
            std::string member = msg.readString();
            Guild *guild = guildManager->findById(id);

            Storage &store = Storage::instance("tmw");
            CharacterPtr ptr = store.getCharacter(playerId);

            if (!guild->checkLeader(ptr.get()))
            {
                // Return that the user doesnt have the rights to invite.
                result.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
                break;
            }

            if (guild->checkInGuild(member))
            {
                // Return that invited member already in guild.
                result.writeByte(ERRMSG_ALREADY_TAKEN);
                break;
            }

            // Send invite to player using chat server
            if (store.doesCharacterNameExist(member))
            {
                sendInvite(member, ptr->getName(), guild->getName());
            }

            guild->addInvited(member);
            result.writeByte(ERRMSG_OK);
        } break;

        case GAMSG_GUILD_ACCEPT:
        {
            // Add accepting into guild
            LOG_DEBUG("Received msg ... GAMSG_GUILD_ACCEPT");
            result.writeShort(AGMSG_GUILD_ACCEPT_RESPONSE);
            int playerId = msg.readLong();
            std::string guildName = msg.readString();
            Guild *guild = guildManager->findByName(guildName);
            if (!guild)
            {
                // Return the guild does not exist.
                result.writeByte(ERRMSG_INVALID_ARGUMENT);
                break;
            }

            Storage &store = Storage::instance("tmw");
            CharacterPtr ptr = store.getCharacter(playerId);

            if (!guild->checkInvited(ptr->getName()))
            {
                // Return the user was not invited.
                result.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
                break;
            }

            if (guild->checkInGuild(ptr->getName()))
            {
                // Return that the player is already in the guild.
                result.writeByte(ERRMSG_ALREADY_TAKEN);
                break;
            }

            result.writeByte(ERRMSG_OK);

            // Who to send data to at the other end
            result.writeLong(playerId);

            // The guild id and guild name they have joined
            result.writeShort(guild->getId());
            result.writeString(guildName);

            // Add member to guild
            guildManager->addGuildMember(guild->getId(), ptr.get());

            // Add guild to character
            ptr->addGuild(guildName);

            // Enter Guild Channel
            enterChannel(guildName, ptr.get());
        } break;

        case GAMSG_GUILD_GET_MEMBERS:
        {
            LOG_DEBUG("Received msg ... GAMSG_GUILD_GET_MEMBERS");
            result.writeShort(AGMSG_GUILD_GET_MEMBERS_RESPONSE);
            int playerId = msg.readLong();
            short guildId = msg.readShort();
            Guild *guild = guildManager->findById(guildId);
            if (!guild)
            {
                result.writeByte(ERRMSG_INVALID_ARGUMENT);
                break;
            }
            result.writeByte(ERRMSG_OK);
            result.writeLong(playerId);
            result.writeShort(guildId);
            for (int i = 0; i < guild->totalMembers(); ++i)
            {
                result.writeString(guild->getMember(i));
            }
        } break;

        case GAMSG_GUILD_QUIT:
        {
            LOG_DEBUG("Received msg ... GAMSG_GUILD_QUIT");
            result.writeShort(AGMSG_GUILD_QUIT_RESPONSE);
            int playerId = msg.readLong();
            short guildId = msg.readShort();
            Guild *guild = guildManager->findById(guildId);
            if (!guild)
            {
                result.writeByte(ERRMSG_INVALID_ARGUMENT);
                break;
            }
            Storage &store = Storage::instance("tmw");
            CharacterPtr ptr = store.getCharacter(playerId);
            guildManager->removeGuildMember(guildId, ptr.get());
            result.writeByte(ERRMSG_OK);
            result.writeLong(playerId);
            result.writeShort(guildId);
        } break;
#endif

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

#if 0
void ServerHandler::enterChannel(const std::string &name,
                                 CharacterData *player)
{
    MessageOut result(CPMSG_ENTER_CHANNEL_RESPONSE);

    short channelId = chatChannelManager->getChannelId(name);
    ChatChannel *channel = chatChannelManager->getChannel(channelId);

    if (!channel)
    {
        // Channel doesn't exist yet so create one
        channelId = chatChannelManager->registerPrivateChannel(name,
                                                               "Guild Channel",
                                                               "");
        channel = chatChannelManager->getChannel(channelId);
    }

    if (channel && channel->addUser(player->getName()))
    {
        result.writeByte(ERRMSG_OK);

        // The user entered the channel, now give him the channel id, the
        // announcement string and the user list.
        result.writeShort(channelId);
        result.writeString(name);
        result.writeString(channel->getAnnouncement());
        const ChatChannel::ChannelUsers &userList = channel->getUserList();

        for (ChatChannel::ChannelUsers::const_iterator i = userList.begin(),
                i_end = userList.end();
                i != i_end; ++i)
        {
            result.writeString(*i);
        }

        // Send an CPMSG_UPDATE_CHANNEL to warn other clients a user went
        // in the channel.
        chatHandler->warnUsersAboutPlayerEventInChat(channel,
                                                     player->getName(),
                                                     CHAT_EVENT_NEW_PLAYER);

    }

    chatHandler->sendGuildEnterChannel(result, player->getName());
}

void ServerHandler::sendInvite(const std::string &invitedName,
                               const std::string &inviterName,
                               const std::string &guildName)
{
    // TODO: Separate account and chat server
    chatHandler->sendGuildInvite(invitedName, inviterName, guildName);
}
#endif
