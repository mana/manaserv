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
#include "game-server/character.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/map.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/state.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "serialize/characterdata.hpp"
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
            ptr->setSpeed(150); // TODO
            ptr->fillHitpoints();// TODO: the current hit points should be saved in the database. Otherwise players could heal their characters by logging in and out again.
            gameHandler->mTokenCollector.addPendingConnect(token, ptr);
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

// The client should directly talk with the chat server and not go through the game server.
#if 0
        case AGMSG_GUILD_CREATE_RESPONSE:
        {
            if(msg.readByte() == ERRMSG_OK)
            {
                int playerId = msg.readLong();
                
                MessageOut result(GPMSG_GUILD_CREATE_RESPONSE);
                result.writeByte(ERRMSG_OK);
                
                /*  Create a message that the player has joined the guild
                 *  Output the guild ID and guild name
                 *  Send a 1 if the player has rights
                 *  to invite users, otherwise 0.
                 */
                MessageOut out(GPMSG_GUILD_JOINED);
                out.writeShort(msg.readShort());
                out.writeString(msg.readString());
                out.writeShort(msg.readShort());
                
                Character *player = gameHandler->messageMap[playerId];
                if(player)
                {
                    gameHandler->sendTo(player, result);
                    gameHandler->sendTo(player, out);
                }
            }
        } break;
            
        case AGMSG_GUILD_INVITE_RESPONSE:
        {
            if(msg.readByte() == ERRMSG_OK)
            {
                int playerId = msg.readLong();
                
                MessageOut result(GPMSG_GUILD_INVITE_RESPONSE);
                result.writeByte(ERRMSG_OK);
                
                Character *player = gameHandler->messageMap[playerId];
                if(player)
                {
                    gameHandler->sendTo(player, result);
                }                
            }
        } break;
            
        case AGMSG_GUILD_ACCEPT_RESPONSE:
        {
            if(msg.readByte() == ERRMSG_OK)
            {
                int playerId = msg.readLong();
                
                MessageOut result(GPMSG_GUILD_ACCEPT_RESPONSE);
                result.writeByte(ERRMSG_OK);
                
                /*  Create a message that the player has joined the guild
                 *  Output the guild ID and guild name
                 *  Send a 0 for invite rights, since player has been invited
                 *  they wont have any rights to invite other users yet.
                 */
                MessageOut out(GPMSG_GUILD_JOINED);
                out.writeShort(msg.readShort());
                out.writeString(msg.readString());
                out.writeShort(0);
                
                Character *player = gameHandler->messageMap[playerId];
                if(player)
                {
                    gameHandler->sendTo(player, result);
                    gameHandler->sendTo(player, out);
                }                
            }
        } break;
            
        case AGMSG_GUILD_GET_MEMBERS_RESPONSE:
        {
            if(msg.readByte() != ERRMSG_OK)
                break;
            int playerId = msg.readLong();
            short guildId = msg.readShort();
            
            MessageOut result(GPMSG_GUILD_GET_MEMBERS_RESPONSE);
            result.writeByte(ERRMSG_OK);
            result.writeShort(guildId);
            while(msg.getUnreadLength())
            {
                result.writeString(msg.readString());
            }
            
            Character *player = gameHandler->messageMap[playerId];
            if(player)
            {
                gameHandler->sendTo(player, result);
            }
        } break;
            
        case AGMSG_GUILD_QUIT_RESPONSE:
        {
            if(msg.readByte() != ERRMSG_OK)
                break;
            int playerId = msg.readLong();
            short guildId = msg.readShort();
            
            MessageOut result(GPMSG_GUILD_QUIT_RESPONSE);
            result.writeByte(ERRMSG_OK);
            result.writeShort(guildId);
            
            Character *player = gameHandler->messageMap[playerId];
            if(player)
            {
                gameHandler->sendTo(player, result);
            }
        } break;
#endif

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

#if 0
void AccountConnection::playerCreateGuild(int id, const std::string &guildName)
{
    LOG_INFO("Send GAMSG_GUILD_CREATE");
    MessageOut msg(GAMSG_GUILD_CREATE);
    msg.writeLong(id);
    msg.writeString(guildName);
    send(msg);
}

void AccountConnection::playerInviteToGuild(int id, short guildId, const std::string &member)
{
    LOG_INFO("Send GAMSG_GUILD_INVITE");
    MessageOut msg(GAMSG_GUILD_INVITE);
    msg.writeLong(id);
    msg.writeShort(guildId);
    msg.writeString(member);
    send(msg);
}

void AccountConnection::playerAcceptInvite(int id, const std::string &guildName)
{
    LOG_INFO("Send GAMSG_GUILD_ACCEPT");
    MessageOut msg(GAMSG_GUILD_ACCEPT);
    msg.writeLong(id);
    msg.writeString(guildName);
    send(msg);
}

void AccountConnection::getGuildMembers(int id, short guildId)
{
    LOG_INFO("Send GAMSG_GUILD_GET_MEMBERS");
    MessageOut msg(GAMSG_GUILD_GET_MEMBERS);
    msg.writeLong(id);
    msg.writeShort(guildId);
    send(msg);
}

void AccountConnection::quitGuild(int id, short guildId)
{
    LOG_INFO("Send GAMSG_GUILD_QUIT");
    MessageOut msg(GAMSG_GUILD_QUIT);
    msg.writeLong(id);
    msg.writeShort(guildId);
    send(msg);
}
#endif
