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

#include "defines.h"
#include "account-server/characterdata.hpp"
#include "account-server/guild.hpp"
#include "account-server/guildmanager.hpp"
#include "account-server/serverhandler.hpp"
#include "chat-server/chatchannelmanager.hpp"
#include "chat-server/chatclient.hpp"
#include "chat-server/chathandler.hpp"
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
    MessageOut msg(GPMSG_CONNECTION_TIMEDOUT);

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
        // sendGuildRejoin(computer);
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

    char channelType = msg.readByte();
    if (!channelType)  // 0 public, 1 private
    {
        if (client.accountLevel != AL_ADMIN &&
                client.accountLevel != AL_GM)
        {
            // Removed the need for admin/gm rights to create public channels
            //reply.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
            //send message
            //return;
        }
    }

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
    else if (guildManager->doesExist(channelName))
    {
        // Channel already exists
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else
    {
        // We attempt to create a new channel
        short channelId;

        // TODO: b_lindeijer: These methods should really be combined.
        if (channelType)
        {
            channelId = chatChannelManager->registerPrivateChannel(
                    channelName,
                    channelAnnouncement,
                    channelPassword);
        }
        else
        {
            channelId = chatChannelManager->registerPublicChannel(
                    channelName,
                    channelAnnouncement,
                    channelPassword);
        }

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
    else if (channelId < (signed) MAX_PUBLIC_CHANNELS_RANGE)
    {
        // Public channel

        // Get character based on name
        CharacterPtr character =
            serverHandler->getCharacter(client.characterName);
        const std::string &channelName = channel->getName();

        if (client.accountLevel == AL_ADMIN || client.accountLevel == AL_GM)
        {
            warnUsersAboutPlayerEventInChat(
                    channel, "", CHAT_EVENT_LEAVING_PLAYER);
            if (chatChannelManager->removeChannel(channelId))
                reply.writeByte(ERRMSG_OK);
            else
                reply.writeByte(ERRMSG_FAILURE);
        }
        else if (guildManager->doesExist(channelName))
        {
            Guild *guild = guildManager->findByName(channelName);
            if (guild->checkLeader(character.get()))
            {
                // TODO: b_lindeijer: I think it would be better if guild
                //        channels were removed in response to a guild being
                //        removed, as opposed to removing a guild because its
                //        channel disappears.
                chatChannelManager->removeChannel(channelId);
                guildManager->removeGuild(guild->getId());
                reply.writeByte(ERRMSG_OK);
            }
            else
            {
                reply.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
            }
        }
        else
        {
            reply.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
        }
    }
    else
    {
        // Private channel

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

    short channelId = chatChannelManager->getChannelId(channelName);
    ChatChannel *channel = chatChannelManager->getChannel(channelId);

    // TODO: b_lindeijer: Currently, the client has to join its guild channels
    //        explicitly by sending 'enter channel' messages. This should be
    //        changed to implicitly joining relevant guild channels right after
    //        login.
    Guild *guild = guildManager->findByName(channelName);

    if (!channelId || !channel)
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (!channel->getPassword().empty() &&
            channel->getPassword() != givenPassword)
    {
        // Incorrect password (should probably have its own return value)
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (guild && !guild->checkInGuild(client.characterName))
    {
        // Player tried to join a guild channel of a guild he's not a member of
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else
    {
        if (channel->addUser(&client))
        {
            // In the case of a guild, send user joined message.
            if (guild)
            {
                sendUserJoined(channel, client.characterName);
            }

            reply.writeByte(ERRMSG_OK);
            // The user entered the channel, now give him the channel
            // id, the announcement string and the user list.
            reply.writeShort(channelId);
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

        // TODO: b_lindeijer: Clients aren't supposed to quit guild
        //        channels explicitly, this should rather happen
        //        implicitly. See similar note at handling 'enter channel'
        //        messages.
        const std::string &channelName = channel->getName();

        if (guildManager->doesExist(channelName))
        {
            // Send a user left message
            sendUserLeft(channel, client.characterName);
        }
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

    // TODO: b_lindeijer: Since it only makes sense to ask for the list of
    //        users in a channel you're in, this message should really take
    //        a channel id instead.
    std::string channelName = msg.readString();

    int channelId = chatChannelManager->getChannelId(channelName);
    ChatChannel *channel = chatChannelManager->getChannel(channelId);

    reply.writeString(channelName);

    if (channel)
    {
        const ChatChannel::ChannelUsers &users = channel->getUserList();

        for (ChatChannel::ChannelUsers::const_iterator
             i = users.begin(), i_end = users.end(); i != i_end; ++i)
        {
            reply.writeString((*i)->characterName);
        }
    }

    client.send(reply);
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

void ChatHandler::sendGuildEnterChannel(const MessageOut &msg,
                                        const std::string &name)
{
    // TODO: b_lindeijer: This method is just an inefficient way to send a
    //        message to a player with a certain name. Would be good to get
    //        rid of it.
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i)
    {
        if (static_cast< ChatClient * >(*i)->characterName == name)
        {
            (*i)->send(msg);
            break;
        }
    }
}

void ChatHandler::sendGuildInvite(const std::string &invitedName,
                                  const std::string &inviterName,
                                  const std::string &guildName)
{
    MessageOut msg(CPMSG_GUILD_INVITED);
    msg.writeString(inviterName);
    msg.writeString(guildName);

    // TODO: b_lindeijer: This is just an inefficient way to send a message to
    //        a player with a certain name. Would be good if the invitedName
    //        could be replaced with a ChatClient.
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i)
    {
        if (static_cast< ChatClient * >(*i)->characterName == invitedName)
        {
            (*i)->send(msg);
            break;
        }
    }
}

#if 0
void ChatHandler::sendGuildRejoin(ChatClient &client)
{
    // Get character based on name.
    CharacterPtr character = serverHandler->getCharacter(client.characterName);

    // Get list of guilds and check what rights they have.
    std::vector<std::string> guilds = character->getGuilds();
    for (unsigned int i = 0; i != guilds.size(); ++i)
    {
        Guild *guild = guildManager->findByName(guilds[i]);
        short leader = 0;
        if (!guild)
        {
            return;
        }
        if (guild->checkLeader(character.get()))
        {
            leader = 1;
        }
        MessageOut msg(CPMSG_GUILD_REJOIN);
        msg.writeString(guild->getName());
        msg.writeShort(guild->getId());
        msg.writeShort(leader);
        client.send(msg);
        serverHandler->enterChannel(guild->getName(), character.get());
    }
}
#endif

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
