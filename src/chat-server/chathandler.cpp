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

#include <list>
#include <algorithm>

#include "defines.h"
#include "chat-server/guild.hpp"
#include "chat-server/guildmanager.hpp"
#include "chat-server/chatchannelmanager.hpp"
#include "chat-server/chatclient.hpp"
#include "chat-server/chathandler.hpp"
#include "chat-server/party.hpp"
#include "net/connectionhandler.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "utils/logger.h"
#include "utils/stringfilter.h"
#include "utils/tokendispenser.hpp"

void registerChatClient(const std::string &token,
                        const std::string &name,
                        int level)
{
    ChatHandler::Pending *p = new ChatHandler::Pending;
    p->character = name;
    p->level = level;
    chatHandler->mTokenCollector.addPendingConnect(token, p);
}

ChatHandler::ChatHandler():
    mTokenCollector(this)
{
}

bool ChatHandler::startListen(enet_uint16 port)
{
    LOG_INFO("Chat handler started:");
    return ConnectionHandler::startListen(port);
}

void ChatHandler::deletePendingClient(ChatClient *c)
{
    MessageOut msg(CPMSG_CONNECT_RESPONSE);
    msg.writeByte(ERRMSG_TIME_OUT);

    // The computer will be deleted when the disconnect event is processed
    c->disconnect(msg);
}

void ChatHandler::deletePendingConnect(Pending *p)
{
    delete p;
}

void ChatHandler::tokenMatched(ChatClient *c, Pending *p)
{
    c->characterName = p->character;
    c->accountLevel = p->level;
    delete p;
    MessageOut msg(CPMSG_CONNECT_RESPONSE);
    msg.writeByte(ERRMSG_OK);
    c->send(msg);

    // Add chat client to player map
    mPlayerMap.insert(std::pair<std::string, ChatClient*>(c->characterName, c));
}

NetComputer *ChatHandler::computerConnected(ENetPeer *peer)
{
    return new ChatClient(peer);
}

void ChatHandler::computerDisconnected(NetComputer *comp)
{
    ChatClient *computer = static_cast< ChatClient * >(comp);

    if (computer->characterName.empty())
    {
        // Not yet fully logged in, remove it from pending clients.
        mTokenCollector.deletePendingClient(computer);
    }
    else
    {
        // Remove user from all channels.
        chatChannelManager->removeUserFromAllChannels(computer);

        // Remove the character from the player map
        mPlayerMap.erase(computer->characterName);

        // Remove user from party
        removeUserFromParty(*computer);
    }

    delete computer;
}

void ChatHandler::processMessage(NetComputer *comp, MessageIn &message)
{
    ChatClient &computer = *static_cast< ChatClient * >(comp);
    MessageOut result;

    if (computer.characterName.empty())
    {
        if (message.getId() != PCMSG_CONNECT) return;

        std::string magic_token = message.readString(MAGIC_TOKEN_LENGTH);
        mTokenCollector.addPendingClient(magic_token, &computer);
        sendGuildRejoin(computer);
        return;
    }

    switch (message.getId())
    {
        case PCMSG_CHAT:
            handleChatMessage(computer, message);
            break;

        case PCMSG_ANNOUNCE:
            handleAnnounceMessage(computer, message);
            break;

        case PCMSG_PRIVMSG:
            handlePrivMsgMessage(computer, message);
            break;

        case PCMSG_REGISTER_CHANNEL:
            handleRegisterChannelMessage(computer, message);
            break;

        case PCMSG_UNREGISTER_CHANNEL:
            handleUnregisterChannelMessage(computer, message);
            break;

        case PCMSG_ENTER_CHANNEL:
            handleEnterChannelMessage(computer, message);
            break;

        case PCMSG_QUIT_CHANNEL:
            handleQuitChannelMessage(computer, message);
            break;

        case PCMSG_LIST_CHANNELS:
            handleListChannelsMessage(computer, message);
            break;

        case PCMSG_LIST_CHANNELUSERS:
            handleListChannelUsersMessage(computer, message);
            break;

        case PCMSG_DISCONNECT:
            handleDisconnectMessage(computer, message);
            break;

        case PCMSG_GUILD_CREATE:
            handleGuildCreation(computer, message);
            break;

        case PCMSG_GUILD_INVITE:
            handleGuildInvitation(computer, message);
            break;

        case PCMSG_GUILD_ACCEPT:
            handleGuildAcceptInvite(computer, message);
            break;

        case PCMSG_GUILD_GET_MEMBERS:
            handleGuildRetrieveMembers(computer, message);
            break;

        case PCMSG_GUILD_QUIT:
            handleGuildQuit(computer, message);
            break;

        case PCMSG_PARTY_INVITE:
            handlePartyInvite(computer, message);
            break;

        case PCMSG_PARTY_ACCEPT_INVITE:
            handlePartyAcceptInvite(computer, message);
            break;

        case PCMSG_PARTY_QUIT:
            handlePartyQuit(computer);

        default:
            LOG_WARN("ChatHandler::processMessage, Invalid message type"
                     << message.getId());
            result.writeShort(XXMSG_INVALID);
            break;
    }

    if (result.getLength() > 0)
        computer.send(result);
}

void
ChatHandler::handleCommand(ChatClient &computer, const std::string &command)
{
    LOG_INFO("Chat: Received unhandled command: " << command);
    MessageOut result;
    result.writeShort(CPMSG_ERROR);
    result.writeByte(CHAT_UNHANDLED_COMMAND);
    computer.send(result);
}

void
ChatHandler::warnPlayerAboutBadWords(ChatClient &computer)
{
    // We could later count if the player is really often unpolite.
    MessageOut result;
    result.writeShort(CPMSG_ERROR);
    result.writeByte(CHAT_USING_BAD_WORDS); // The Channel
    computer.send(result);

    LOG_INFO(computer.characterName << " says bad words.");
}

void
ChatHandler::handleChatMessage(ChatClient &client, MessageIn &msg)
{
    std::string text = msg.readString();

    // Pass it through the slang filter (false when it contains bad words)
    if (!stringFilter->filterContent(text))
    {
        warnPlayerAboutBadWords(client);
        return;
    }

    short channelId = msg.readShort();
    ChatChannel *channel = chatChannelManager->getChannel(channelId);

    if (channel)
    {
        LOG_DEBUG(client.characterName << " says in channel " << channelId
                  << ": " << text);

        MessageOut result(CPMSG_PUBMSG);
        result.writeShort(channelId);
        result.writeString(client.characterName);
        result.writeString(text);
        sendInChannel(channel, result);
    }
}

void
ChatHandler::handleAnnounceMessage(ChatClient &client, MessageIn &msg)
{
    std::string text = msg.readString();

    if (!stringFilter->filterContent(text))
    {
        warnPlayerAboutBadWords(client);
        return;
    }

    if (client.accountLevel == AL_ADMIN || client.accountLevel == AL_GM)
    {
        // TODO: b_lindeijer: Shouldn't announcements also have a sender?
        LOG_INFO("ANNOUNCE: " << text);
        MessageOut result(CPMSG_ANNOUNCEMENT);
        result.writeString(text);

        // We send the message to all players in the default channel as it is
        // an announcement.
        sendToEveryone(result);
    }
    else
    {
        MessageOut result(CPMSG_ERROR);
        result.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
        client.send(result);
        LOG_INFO(client.characterName <<
            " couldn't make an announcement due to insufficient rights.");
    }
}

void
ChatHandler::handlePrivMsgMessage(ChatClient &client, MessageIn &msg)
{
    std::string user = msg.readString();
    std::string text = msg.readString();

    if (!stringFilter->filterContent(text))
    {
        warnPlayerAboutBadWords(client);
        return;
    }

    // We seek the player to whom the message is told and send it to her/him.
    sayToPlayer(client, user, text);
}

void
ChatHandler::handleRegisterChannelMessage(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_REGISTER_CHANNEL_RESPONSE);

    std::string channelName = msg.readString();
    std::string channelAnnouncement = msg.readString();
    std::string channelPassword = msg.readString();

    if (!stringFilter->filterContent(channelName) ||
            !stringFilter->filterContent(channelAnnouncement))
    {
        warnPlayerAboutBadWords(client);
        return;
    }

    // Checking strings for length and double quotes
    if (channelName.empty() ||
            channelName.length() > MAX_CHANNEL_NAME ||
            stringFilter->findDoubleQuotes(channelName) ||
            channelAnnouncement.length() > MAX_CHANNEL_ANNOUNCEMENT ||
            stringFilter->findDoubleQuotes(channelAnnouncement) ||
            channelPassword.length() > MAX_CHANNEL_PASSWORD ||
            stringFilter->findDoubleQuotes(channelPassword))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (guildManager->doesExist(channelName) || chatChannelManager->channelExists(channelName))
    {
        // Channel already exists
        reply.writeByte(ERRMSG_ALREADY_TAKEN);
    }
    else
    {
        // We attempt to create a new channel
        short channelId;

        channelId = chatChannelManager->createNewChannel(
                    channelName,
                    channelAnnouncement,
                    channelPassword,
                    true);

        if (channelId)
        {
            // We add the player as admin of this channel as he created it. The
            // user registering a private channel is the only one to be able to
            // update the password and the announcement in it and also to
            // remove it.
            ChatChannel *channel = chatChannelManager->getChannel(channelId);
            channel->addUser(&client);

            reply.writeByte(ERRMSG_OK);
            reply.writeShort(channelId);
            reply.writeString(channelName);
        }
        else
        {
            reply.writeByte(ERRMSG_FAILURE);
        }
    }

    client.send(reply);
}

void
ChatHandler::handleUnregisterChannelMessage(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_UNREGISTER_CHANNEL_RESPONSE);

    short channelId = msg.readShort();
    ChatChannel *channel = chatChannelManager->getChannel(channelId);

    if (!channel)
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (!channel->canJoin())
    {
        reply.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
    }
    else
    {
        // We first see if the user is the admin (first user) of the channel
        const ChatChannel::ChannelUsers &userList = channel->getUserList();
        ChatChannel::ChannelUsers::const_iterator i = userList.begin();

        if (*i != &client)
        {
            reply.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
        }
        else
        {
            // Make every user quit the channel
            warnUsersAboutPlayerEventInChat(
                    channel, "", CHAT_EVENT_LEAVING_PLAYER);

            if (chatChannelManager->removeChannel(channelId))
            {
                reply.writeByte(ERRMSG_OK);
            }
            else
            {
                reply.writeByte(ERRMSG_FAILURE);
            }
        }
    }

    client.send(reply);
}

void ChatHandler::handleEnterChannelMessage(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_ENTER_CHANNEL_RESPONSE);

    std::string channelName = msg.readString();
    std::string givenPassword = msg.readString();

    ChatChannel *channel = chatChannelManager->getChannel(channelName);

    if (!channel)
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (!channel->getPassword().empty() &&
            channel->getPassword() != givenPassword)
    {
        // Incorrect password (should probably have its own return value)
        reply.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
    }
    else if (!channel->canJoin())
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else
    {
        if (channel->addUser(&client))
        {
            reply.writeByte(ERRMSG_OK);
            // The user entered the channel, now give him the channel
            // id, the announcement string and the user list.
            reply.writeShort(channel->getId());
            reply.writeString(channelName);
            reply.writeString(channel->getAnnouncement());
            const ChatChannel::ChannelUsers &users = channel->getUserList();

            for (ChatChannel::ChannelUsers::const_iterator i = users.begin(),
                    i_end = users.end();
                    i != i_end; ++i)
            {
                reply.writeString((*i)->characterName);
            }
            // Send an CPMSG_UPDATE_CHANNEL to warn other clients a user went
            // in the channel.
            warnUsersAboutPlayerEventInChat(channel,
                    client.characterName,
                    CHAT_EVENT_NEW_PLAYER);
        }
        else
        {
            reply.writeByte(ERRMSG_FAILURE);
        }
    }

    client.send(reply);
}

void
ChatHandler::handleQuitChannelMessage(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_QUIT_CHANNEL_RESPONSE);

    short channelId = msg.readShort();
    ChatChannel *channel = chatChannelManager->getChannel(channelId);

    if (channelId == 0 || !channel)
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (!channel->removeUser(&client))
    {
        reply.writeByte(ERRMSG_FAILURE);
    }
    else
    {
        reply.writeByte(ERRMSG_OK);
        reply.writeShort(channelId);

        // Send an CPMSG_UPDATE_CHANNEL to warn other clients a user left
        // the channel.
        warnUsersAboutPlayerEventInChat(channel,
                client.characterName,
                CHAT_EVENT_LEAVING_PLAYER);
    }

    client.send(reply);
}

void
ChatHandler::handleListChannelsMessage(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_LIST_CHANNELS_RESPONSE);

    std::list<const ChatChannel*> channels =
        chatChannelManager->getPublicChannels();

    for (std::list<const ChatChannel*>::iterator i = channels.begin(),
            i_end = channels.end();
            i != i_end; ++i)
    {
        const std::string &name = (*i)->getName();
        short users = (*i)->getUserList().size();
        reply.writeString(name);
        reply.writeShort(users);
    }

    client.send(reply);
}

void
ChatHandler::handleListChannelUsersMessage(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_LIST_CHANNELUSERS_RESPONSE);

    std::string channelName = msg.readString();
    ChatChannel *channel = chatChannelManager->getChannel(channelName);

    if (channel)
    {
        reply.writeString(channel->getName());

        const ChatChannel::ChannelUsers &users = channel->getUserList();

        for (ChatChannel::ChannelUsers::const_iterator
             i = users.begin(), i_end = users.end(); i != i_end; ++i)
        {
            reply.writeString((*i)->characterName);
        }

        client.send(reply);
    }
}

void
ChatHandler::handleDisconnectMessage(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_DISCONNECT_RESPONSE);
    reply.writeByte(ERRMSG_OK);
    chatChannelManager->removeUserFromAllChannels(&client);
    client.send(reply);
}

void
ChatHandler::handleGuildCreation(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_GUILD_CREATE_RESPONSE);

    // Check if guild already exists and if so, return error
    std::string guildName = msg.readString();
    if (!guildManager->doesExist(guildName))
    {
        // Guild doesnt already exist so create it
        Guild *guild = guildManager->createGuild(guildName, client.characterName);
        reply.writeByte(ERRMSG_OK);
        reply.writeString(guildName);
        reply.writeShort(guild->getId());
        reply.writeByte(true);

        // Send autocreated channel id
        short channelId = joinGuildChannel(guildName, client);
        reply.writeShort(channelId);
    }
    else
    {
        reply.writeByte(ERRMSG_ALREADY_TAKEN);
    }

    client.send(reply);
}

void
ChatHandler::handleGuildInvitation(ChatClient &client, MessageIn &msg)
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
        if (guild->checkLeader(client.characterName) &&
            (client.characterName != character) &&
            !guild->checkInGuild(character))
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
            guild->addInvited(character);
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

void
ChatHandler::handleGuildAcceptInvite(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_GUILD_ACCEPT_RESPONSE);
    std::string guildName = msg.readString();

    // check guild exists and that member was invited
    // then add them as guild member
    // and remove from invite list
    Guild *guild = guildManager->findByName(guildName);
    if (guild)
    {
        if (guild->checkInvited(client.characterName))
        {
            guild->addMember(client.characterName);
            reply.writeByte(ERRMSG_OK);
            reply.writeString(guild->getName());
            reply.writeShort(guild->getId());
            reply.writeByte(false);

            short id = joinGuildChannel(guild->getName(), client);
            reply.writeShort(id);
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

void
ChatHandler::handleGuildRetrieveMembers(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_GUILD_GET_MEMBERS_RESPONSE);
    short guildId = msg.readShort();
    Guild *guild = guildManager->findById(guildId);

    // check for valid guild
    // write a list of member names that belong to the guild
    if (guild)
    {
        // make sure the requestor is in the guild
        if (guild->checkInGuild(client.characterName))
        {
            reply.writeByte(ERRMSG_OK);
            reply.writeShort(guildId);
            for(int i = 0; i < guild->totalMembers(); ++i)
            {
                reply.writeString(guild->getMember(i));
            }
        }
    }
    else
    {
        reply.writeByte(ERRMSG_FAILURE);
    }

    client.send(reply);
}

void
ChatHandler::handleGuildQuit(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_GUILD_QUIT_RESPONSE);
    short guildId = msg.readShort();
    Guild *guild = guildManager->findById(guildId);

    // check for valid guild
    // check the member is in the guild
    // remove the member from the guild
    if (guild)
    {
        if (guild->checkInGuild(client.characterName))
        {
            reply.writeByte(ERRMSG_OK);
            reply.writeShort(guildId);

            // Check if they are the leader, and if so, remove the guild channel
            if (guild->checkLeader(client.characterName))
            {
                chatChannelManager->removeChannel(chatChannelManager->getChannelId(guild->getName()));
            }

            guildManager->removeGuildMember(guildId, client.characterName);
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

void
ChatHandler::sayToPlayer(ChatClient &computer, const std::string &playerName,
                         const std::string &text)
{
    MessageOut result;
    LOG_DEBUG(computer.characterName << " says to " << playerName << ": "
              << text);
    // Send it to the being if the being exists
    result.writeShort(CPMSG_PRIVMSG);
    result.writeString(computer.characterName);
    result.writeString(text);
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i) {
        if (static_cast< ChatClient * >(*i)->characterName == playerName)
        {
            (*i)->send(result);
            break;
        }
    }
}

void ChatHandler::warnUsersAboutPlayerEventInChat(ChatChannel *channel,
                                                  const std::string &userName,
                                                  char eventId)
{
    MessageOut msg(CPMSG_CHANNEL_EVENT);
    msg.writeShort(channel->getId());
    msg.writeByte(eventId);
    msg.writeString(userName);
    sendInChannel(channel, msg);
}

void ChatHandler::sendInChannel(ChatChannel *channel, MessageOut &msg)
{
    const ChatChannel::ChannelUsers &users = channel->getUserList();

    for (ChatChannel::ChannelUsers::const_iterator
         i = users.begin(), i_end = users.end(); i != i_end; ++i)
    {
        (*i)->send(msg);
    }
}

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
    std::vector<Guild*> guilds = guildManager->getGuilds(client.characterName);
    for (unsigned int i = 0; i != guilds.size(); ++i)
    {
        Guild *guild = guilds[i];
        short leader = 0;
        if (!guild)
        {
            return;
        }
        if (guild->checkLeader(client.characterName))
        {
            leader = 1;
        }

        std::string guildName = guild->getName();

        // Tell the client what guilds the character belongs to and their permissions
        MessageOut msg(CPMSG_GUILD_REJOIN);
        msg.writeString(guildName);
        msg.writeShort(guild->getId());
        msg.writeByte(leader);

        // get channel id of guild channel
        short channelId = joinGuildChannel(guildName, client);

        // send the channel id for the autojoined channel
        msg.writeShort(channelId);

        client.send(msg);

        sendGuildListUpdate(guildName, client.characterName);

    }
}

void ChatHandler::sendUserJoined(ChatChannel *channel, const std::string &name)
{
    MessageOut msg(CPMSG_USERJOINED);
    msg.writeShort(channel->getId());
    msg.writeString(name);
    sendInChannel(channel, msg);
}

void ChatHandler::sendUserLeft(ChatChannel *channel, const std::string &name)
{
    MessageOut msg(CPMSG_USERLEFT);
    msg.writeShort(channel->getId());
    msg.writeString(name);
    sendInChannel(channel, msg);
}

int ChatHandler::joinGuildChannel(const std::string &guildName, ChatClient &client)
{
    int channelId = 0;
    // Automatically make the character join the guild chat channel
    ChatChannel *channel = chatChannelManager->getChannel(guildName);
    if (!channel)
    {
        // Channel doesnt exist so create it
        channelId = chatChannelManager->createNewChannel(guildName,
                "Guild Channel", "", false);
        channel = chatChannelManager->getChannel(channelId);
    }
    else
    {
        channelId = channel->getId();
    }

    // Add user to the channel
    if (channel->addUser(&client))
    {
        // Send an CPMSG_UPDATE_CHANNEL to warn other clients a user went
        // in the channel.
        warnUsersAboutPlayerEventInChat(channel, client.characterName,
                CHAT_EVENT_NEW_PLAYER);

        sendGuildListUpdate(guildName, client.characterName);
    }

    return channelId;
}

void ChatHandler::sendGuildListUpdate(const std::string &guildName,
                                      const std::string &characterName)
{
    Guild *guild = guildManager->findByName(guildName);
    if (guild)
    {
        MessageOut msg(CPMSG_GUILD_UPDATE_LIST);

        msg.writeShort(guild->getId());
        msg.writeString(characterName);

        // TODO: This should get a list of all members
        // and iterate through them
        std::map<std::string, ChatClient*>::iterator itr;
        for (int i = 0; i < guild->totalMembers(); ++i)
        {
            itr = mPlayerMap.find(guild->getMember(i));
            if (itr != mPlayerMap.end())
            {
                itr->second->send(msg);
            }
        }
    }
}

bool ChatHandler::handlePartyJoin(const std::string &invited, const std::string &inviter)
{
    MessageOut out(CPMSG_PARTY_INVITE_RESPONSE);

    // Get inviting client
    ChatClient *c1 = getClient(inviter);
    if (c1)
    {
        // if party doesnt exist, create it
        if (!c1->party)
        {
            c1->party = new Party();
        }

        // add inviter to the party
        c1->party->addUser(inviter);

        // Get invited client
        ChatClient *c2 = getClient(invited);
        if (c2)
        {
            // add invited to the party
            c1->party->addUser(invited);
            c2->party = c1->party;
            // was successful so return success to inviter
            out.writeString(invited);
            out.writeByte(ERRMSG_OK);
            c1->send(out);
            return true;
        }
    }

    // there was an error, return false
    return false;

}

void ChatHandler::handlePartyInvite(ChatClient &client, MessageIn &msg)
{
    //TODO: Handle errors
    MessageOut out(CPMSG_PARTY_INVITED);

    out.writeString(client.characterName);

    std::string invited = msg.readString();
    if (invited != "")
    {
        // Get client and send it the invite
        ChatClient *c = getClient(invited);
        if (c)
        {
            // store the invite
            mPartyInvitedUsers.push_back(invited);
            c->send(out);
        }
    }
}

void ChatHandler::handlePartyAcceptInvite(ChatClient &client, MessageIn &msg)
{
    MessageOut out(CPMSG_PARTY_ACCEPT_INVITE_RESPONSE);

    // Check that the player was invited
    std::vector<std::string>::iterator itr;
    itr = std::find(mPartyInvitedUsers.begin(), mPartyInvitedUsers.end(),
                    client.characterName);
    if (itr != mPartyInvitedUsers.end())
    {
        // make them join the party
        if (handlePartyJoin(client.characterName, msg.readString()))
        {
            out.writeByte(ERRMSG_OK);
            mPartyInvitedUsers.erase(itr);
        }
        else
        {
            out.writeByte(ERRMSG_FAILURE);
        }
    }
    else
    {
        out.writeByte(ERRMSG_FAILURE);
    }
    client.send(out);
}

void ChatHandler::handlePartyQuit(ChatClient &client)
{
    removeUserFromParty(client);
    MessageOut out(CPMSG_PARTY_QUIT_RESPONSE);
    out.writeByte(ERRMSG_OK);
    client.send(out);
}

void ChatHandler::removeUserFromParty(ChatClient &client)
{
    if (client.party)
    {
        client.party->removeUser(client.characterName);
        if (client.party->numUsers() < 1)
        {
            delete client.party;
            client.party = 0;
        }
    }
}

ChatClient* ChatHandler::getClient(const std::string &name)
{
    std::map<std::string, ChatClient*>::iterator itr;
    itr = mPlayerMap.find(name);
    if (itr != mPlayerMap.end())
    {
        return itr->second;
    }
    else
    {
        return NULL;
    }
}
