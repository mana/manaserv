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

#include "account-server/accountclient.hpp"
#include "account-server/characterdata.hpp"
#include "account-server/guildmanager.hpp"
#include "account-server/serverhandler.hpp"
#include "account-server/storage.hpp"
#include "chat-server/chathandler.hpp"
#include "chat-server/chatchannelmanager.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "utils/logger.h"
#include "utils/tokendispenser.hpp"
#include "utils/tokencollector.hpp"

bool ServerHandler::startListen(enet_uint16 port)
{
    LOG_INFO("Game server handler started:");
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
            LOG_INFO("Unregistering map " << i->first << '.');
            servers.erase(i++);
        }
        else
        {
            ++i;
        }
    }
    delete comp;
}

bool ServerHandler::getGameServerFromMap(unsigned mapId, std::string &address,
                                         short &port)
{
    Servers::const_iterator i = servers.find(mapId);
    if (i == servers.end()) return false;
    address = i->second.address;
    port = i->second.port;
    return true;
}

void ServerHandler::registerGameClient(std::string const &token, CharacterPtr ptr)
{
    unsigned mapId = ptr->getMapId();

    MessageOut msg(AGMSG_PLAYER_ENTER);
    msg.writeString(token, MAGIC_TOKEN_LENGTH);
    ptr->serialize(msg); //Characterdata

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
            LOG_DEBUG("GAMSG_REGISTER");
            // TODO: check the credentials of the game server
            std::string address = msg.readString();
            int port = msg.readShort();
            Server s = { address, port, comp };
            LOG_INFO("Game server " << address << ':' << port
                     << " wants to register " << (msg.getUnreadLength() / 2)
                     << " maps.");

            while (msg.getUnreadLength())
            {
                int id = msg.readShort();
                LOG_INFO("Registering map " << id << '.');
                if (servers.insert(std::make_pair(id, s)).second)
                {
                    MessageOut outMsg(AGMSG_ACTIVE_MAP);
                    outMsg.writeShort(id);
                    comp->send(outMsg);
                }
                else
                {
                    LOG_ERROR("Server Handler: map is already registered.");
                }
            }
        } break;

        case GAMSG_PLAYER_DATA:
        {
            LOG_DEBUG("GAMSG_PLAYER_DATA");
            // TODO: Store it in memory, only update the database when needed.
            //       That should get rid of the
            //       no_update_on_switch_character_bug as well.
            Storage &store = Storage::instance("tmw");
            CharacterPtr ptr(new CharacterData(msg));

            if (!store.updateCharacter(ptr))
                        LOG_ERROR("Received character data for non-existing" <<
                                  " character " << ptr->getDatabaseID() << ".");

        } break;

        case GAMSG_REDIRECT:
        {
            LOG_DEBUG("GAMSG_REDIRECT");
            int id = msg.readLong();
            std::string magic_token(utils::getMagicToken());
            Storage &store = Storage::instance("tmw");
            CharacterPtr ptr = store.getCharacter(id);
            std::string address;
            short port;
            if (serverHandler->getGameServerFromMap(ptr->getMapId(), address,
                                                    port))
            {
                registerGameClient(magic_token, ptr);
                result.writeShort(AGMSG_REDIRECT_RESPONSE);
                result.writeLong(ptr->getDatabaseID());
                result.writeString(magic_token, MAGIC_TOKEN_LENGTH);
                result.writeString(address);
                result.writeShort(port);
            }
            else
            {
                LOG_ERROR("Server Change: No game server for map " <<
                          ptr->getMapId() << ".");
            }
        } break;

        case GAMSG_PLAYER_RECONNECT:
        {
            LOG_DEBUG("GAMSG_PLAYER_RECONNECT");
            int characterID = msg.readLong();
            std::string magic_token = msg.readString(MAGIC_TOKEN_LENGTH);

            Storage &store = Storage::instance("tmw");
            CharacterPtr ptr = store.getCharacter(characterID);

            int accountID = ptr->getAccountID();
            accountHandler->
                    mTokenCollector.addPendingConnect(magic_token, accountID);

        } break;
            
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

void ServerHandler::enterChannel(const std::string &name, CharacterData *player)
{
    MessageOut result(CPMSG_ENTER_CHANNEL_RESPONSE);
    short channelId = chatChannelManager->getChannelId(name);
    if (!chatChannelManager->isChannelRegistered(channelId))
    {
        // Channel doesnt exist yet so create one
        channelId = chatChannelManager->registerPrivateChannel(
                                            name,
                                            "Guild Channel",
                                            "");
    }
    
    if (chatChannelManager->addUserInChannel(player->getName(), channelId))
    {
        result.writeByte(ERRMSG_OK);
        
        // The user entered the channel, now give him the channel id, the announcement string
        // and the user list.
        result.writeShort(channelId);
        result.writeString(name);
        result.writeString(chatChannelManager->getChannelAnnouncement(channelId));
        std::vector< std::string > const &userList = 
            chatChannelManager->getUserListInChannel(channelId);
        for (std::vector< std::string >::const_iterator i = userList.begin(),
                i_end = userList.end();
                i != i_end; ++i)
        {
            result.writeString(*i);
        }
        
        // Send an CPMSG_UPDATE_CHANNEL to warn other clients a user went
        // in the channel.
        chatHandler->warnUsersAboutPlayerEventInChat(channelId,
                                        player->getName(),
                                        CHAT_EVENT_NEW_PLAYER);

    }
    
    chatHandler->sendGuildEnterChannel(result, player->getName());
}

void ServerHandler::sendInvite(const std::string &invitedName, const std::string &inviterName,
                               const std::string &guildName)
{
    // TODO: Separate account and chat server
    chatHandler->sendGuildInvite(invitedName, inviterName, guildName);
}

CharacterPtr ServerHandler::getCharacter(const std::string &name)
{
    Storage &store = Storage::instance("tmw");
    CharacterPtr character = store.getCharacter(name);
    return character;
}
