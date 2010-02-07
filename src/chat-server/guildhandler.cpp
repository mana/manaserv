/*
 *  The Mana Server
 *  Copyright (C) 2008-2010  The Mana World Development Team
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

#include "chathandler.hpp"
#include "chatchannel.hpp"
#include "chatchannelmanager.hpp"
#include "chatclient.hpp"
#include "guild.hpp"
#include "guildmanager.hpp"

#include "account-server/character.hpp"
#include "account-server/storage.hpp"

#include "net/messagein.hpp"
#include "net/messageout.hpp"

#include "protocol.h"

void ChatHandler::sendGuildInvite(const std::string &invitedName,
                                  const std::string &inviterName,
                                  const std::string &guildName)
{
    MessageOut msg(CPMSG_GUILD_INVITED);
    msg.writeString(inviterName);
    msg.writeString(guildName);

    std::map<std::string, ChatClient*>::iterator itr = mPlayerMap.find(invitedName);
    if (itr == mPlayerMap.end())
    {
        itr->second->send(msg);
    }
}

void ChatHandler::sendGuildRejoin(ChatClient &client)
{
    // Get list of guilds and check what rights they have.
    std::vector<Guild*> guilds = guildManager->getGuildsForPlayer(client.characterId);
    for (unsigned int i = 0; i != guilds.size(); ++i)
    {
        Guild *guild = guilds[i];
        short permissions;
        if (!guild)
        {
            return;
        }
        permissions = guild->getUserPermissions(client.characterId);

        std::string guildName = guild->getName();

        // Tell the client what guilds the character belongs to and their permissions
        MessageOut msg(CPMSG_GUILD_REJOIN);
        msg.writeString(guildName);
        msg.writeShort(guild->getId());
        msg.writeShort(permissions);

        // get channel id of guild channel
        ChatChannel *channel = joinGuildChannel(guildName, client);

        // send the channel id for the autojoined channel
        msg.writeShort(channel->getId());
        msg.writeString(channel->getAnnouncement());

        client.send(msg);

        sendGuildListUpdate(guildName, client.characterName, GUILD_EVENT_ONLINE_PLAYER);

    }
}

ChatChannel *ChatHandler::joinGuildChannel(const std::string &guildName, ChatClient &client)
{
    // Automatically make the character join the guild chat channel
    ChatChannel *channel = chatChannelManager->getChannel(guildName);
    if (!channel)
    {
        // Channel doesnt exist so create it
        int channelId = chatChannelManager->createNewChannel(guildName,
                "Guild Channel", "", false);
        channel = chatChannelManager->getChannel(channelId);
    }

    // Add user to the channel
    if (channel->addUser(&client))
    {
        // Send an CPMSG_UPDATE_CHANNEL to warn other clients a user went
        // in the channel.
        warnUsersAboutPlayerEventInChat(channel, client.characterName,
                CHAT_EVENT_NEW_PLAYER);
    }

    return channel;
}

void ChatHandler::sendGuildListUpdate(const std::string &guildName,
                                      const std::string &characterName,
                                      char eventId)
{
    Guild *guild = guildManager->findByName(guildName);
    if (guild)
    {
        MessageOut msg(CPMSG_GUILD_UPDATE_LIST);

        msg.writeShort(guild->getId());
        msg.writeString(characterName);
        msg.writeByte(eventId);
        std::map<std::string, ChatClient*>::const_iterator chr;
        std::list<GuildMember*> members = guild->getMembers();

        for (std::list<GuildMember*>::const_iterator itr = members.begin();
             itr != members.end(); ++itr)
        {
            Character *c = storage->getCharacter((*itr)->mId, NULL);
            chr = mPlayerMap.find(c->getName());
            if (chr != mPlayerMap.end())
            {
                chr->second->send(msg);
            }
        }
    }
}

void ChatHandler::handleGuildCreation(ChatClient &client,
                                      MessageIn &msg)
{
    MessageOut reply(CPMSG_GUILD_CREATE_RESPONSE);

    // Check if guild already exists and if so, return error
    std::string guildName = msg.readString();
    if (!guildManager->doesExist(guildName))
    {
        // check the player hasnt already created a guild
        if (guildManager->alreadyOwner(client.characterId))
        {
            reply.writeByte(ERRMSG_LIMIT_REACHED);
        }
        else
        {
            // Guild doesnt already exist so create it
            Guild *guild = guildManager->createGuild(guildName, client.characterId);
            reply.writeByte(ERRMSG_OK);
            reply.writeString(guildName);
            reply.writeShort(guild->getId());
            reply.writeShort(guild->getUserPermissions(client.characterId));

            // Send autocreated channel id
            ChatChannel* channel = joinGuildChannel(guildName, client);
            reply.writeShort(channel->getId());
        }
    }
    else
    {
        reply.writeByte(ERRMSG_ALREADY_TAKEN);
    }

    client.send(reply);
}

void ChatHandler::handleGuildInvitation(ChatClient &client,
                                        MessageIn &msg)
{
    MessageOut reply(CPMSG_GUILD_INVITE_RESPONSE);
    MessageOut invite(CPMSG_GUILD_INVITED);

    // send an invitation from sender to character to join guild
    int guildId = msg.readShort();
    std::string character = msg.readString();

    // get the chat client and the guild
    ChatClient *invitedClient = mPlayerMap[character];
    Guild *guild = guildManager->findById(guildId);

    if (invitedClient && guild)
    {
        // check permissions of inviter, and that they arent inviting themself,
        // and arent someone already in the guild
        if (guild->canInvite(client.characterId) &&
            (client.characterName != character) &&
            !guild->checkInGuild(invitedClient->characterId))
        {
            // send the name of the inviter and the name of the guild
            // that the character has been invited to join
            std::string senderName = client.characterName;
            std::string guildName = guild->getName();
            invite.writeString(senderName);
            invite.writeString(guildName);
            invite.writeShort(guildId);
            invitedClient->send(invite);
            reply.writeByte(ERRMSG_OK);

            // add member to list of invited members to the guild
            guild->addInvited(invitedClient->characterId);
        }
        else
        {
            reply.writeByte(ERRMSG_FAILURE);
        }
    }
    else
    {
        reply.writeByte(ERRMSG_FAILURE);
    }

    client.send(reply);
}

void ChatHandler::handleGuildAcceptInvite(ChatClient &client,
                                          MessageIn &msg)
{
    MessageOut reply(CPMSG_GUILD_ACCEPT_RESPONSE);
    std::string guildName = msg.readString();
    bool error = true; // set true by default, and set false only if success

    // check guild exists and that member was invited
    // then add them as guild member
    // and remove from invite list
    Guild *guild = guildManager->findByName(guildName);
    if (guild)
    {
        if (guild->checkInvited(client.characterId))
        {
            // add user to guild
            guildManager->addGuildMember(guild, client.characterId);
            reply.writeByte(ERRMSG_OK);
            reply.writeString(guild->getName());
            reply.writeShort(guild->getId());
            reply.writeShort(guild->getUserPermissions(client.characterId));

            // have character join guild channel
            ChatChannel *channel = joinGuildChannel(guild->getName(), client);
            reply.writeShort(channel->getId());
            sendGuildListUpdate(guildName, client.characterName, GUILD_EVENT_NEW_PLAYER);

            // success! set error to false
            error = false;
        }
    }

    if (error)
    {
        reply.writeByte(ERRMSG_FAILURE);
    }

    client.send(reply);
}

void ChatHandler::handleGuildRetrieveMembers(ChatClient &client,
                                             MessageIn &msg)
{
    MessageOut reply(CPMSG_GUILD_GET_MEMBERS_RESPONSE);
    short guildId = msg.readShort();
    Guild *guild = guildManager->findById(guildId);

    // check for valid guild
    // write a list of member names that belong to the guild
    if (guild)
    {
        // make sure the requestor is in the guild
        if (guild->checkInGuild(client.characterId))
        {
            reply.writeByte(ERRMSG_OK);
            reply.writeShort(guildId);
            std::list<GuildMember*> memberList = guild->getMembers();
            std::list<GuildMember*>::const_iterator itr_end = memberList.end();
            for (std::list<GuildMember*>::iterator itr = memberList.begin();
                 itr != itr_end; ++itr)
            {
                Character *c = storage->getCharacter((*itr)->mId, NULL);
                std::string memberName = c->getName();
                reply.writeString(memberName);
                reply.writeByte(mPlayerMap.find(memberName) != mPlayerMap.end());
            }
        }
    }
    else
    {
        reply.writeByte(ERRMSG_FAILURE);
    }

    client.send(reply);
}

void ChatHandler::handleGuildMemberLevelChange(ChatClient &client,
                                               MessageIn &msg)
{
    // get the guild, the user to change the permissions, and the new permission
    // check theyre valid, and then change them
    MessageOut reply(CPMSG_GUILD_PROMOTE_MEMBER_RESPONSE);
    short guildId = msg.readShort();
    std::string user = msg.readString();
    short level = msg.readByte();
    Guild *guild = guildManager->findById(guildId);
    Character *c = storage->getCharacter(user);

    if (guild && c)
    {
        int rights = guild->getUserPermissions(c->getDatabaseID()) | level;
        if (guildManager->changeMemberLevel(&client, guild, c->getDatabaseID(), rights) == 0)
        {
            reply.writeByte(ERRMSG_OK);
            client.send(reply);
        }
    }

    reply.writeByte(ERRMSG_FAILURE);
    client.send(reply);
}

void ChatHandler::handleGuildMemberKick(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_GUILD_KICK_MEMBER_RESPONSE);
    short guildId = msg.readShort();
    std::string user = msg.readString();

    Guild *guild = guildManager->findById(guildId);
    Character *c = storage->getCharacter(user);

    if (guild && c)
    {
        if (guild->getUserPermissions(c->getDatabaseID()) & GAL_KICK)
        {
            reply.writeByte(ERRMSG_OK);
        }
        else
        {
            reply.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
        }
    }
    else
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }

    client.send(reply);
}

void ChatHandler::handleGuildQuit(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_GUILD_QUIT_RESPONSE);
    short guildId = msg.readShort();
    Guild *guild = guildManager->findById(guildId);

    // check for valid guild
    // check the member is in the guild
    // remove the member from the guild
    if (guild)
    {
        if (guild->checkInGuild(client.characterId))
        {
            reply.writeByte(ERRMSG_OK);
            reply.writeShort(guildId);

            // Check if there are no members left, remove the guild channel
            if (guild->memberCount() == 0)
            {
                chatChannelManager->removeChannel(chatChannelManager->getChannelId(guild->getName()));
            }

            // guild manager checks if the member is the last in the guild
            // and removes the guild if so
            guildManager->removeGuildMember(guild, client.characterId);
            sendGuildListUpdate(guild->getName(), client.characterName, GUILD_EVENT_LEAVING_PLAYER);
        }
        else
        {
            reply.writeByte(ERRMSG_FAILURE);
        }
    }
    else
    {
        reply.writeByte(ERRMSG_FAILURE);
    }

    client.send(reply);
}

void ChatHandler::guildChannelTopicChange(ChatChannel *channel, int playerId,
                                          const std::string &topic)
{
    Guild *guild = guildManager->findByName(channel->getName());
    if (guild && guild->getUserPermissions(playerId) & GAL_TOPIC_CHANGE)
    {
        chatChannelManager->setChannelTopic(channel->getId(), topic);
    }
}
