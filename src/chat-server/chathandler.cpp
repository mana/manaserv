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


struct ChatPendingLogin
{
    std::string character;
    AccountLevel level;
    int timeout;
};

typedef std::map< std::string, ChatPendingLogin > ChatPendingLogins;
static ChatPendingLogins pendingLogins;

typedef std::map< std::string, ChatClient * > ChatPendingClients;
static ChatPendingClients pendingClients;

void registerChatClient(const std::string &token,
                        const std::string &name,
                        int level)
{
    ChatPendingClients::iterator i = pendingClients.find(token);
    if (i != pendingClients.end())
    {
        ChatClient *computer = i->second;
        computer->characterName = name;
        computer->accountLevel = (AccountLevel) level;
        pendingClients.erase(i);
        MessageOut result;
        result.writeShort(CPMSG_CONNECT_RESPONSE);
        result.writeByte(ERRMSG_OK);
        computer->send(result);
    }
    else
    {
        ChatPendingLogin p;
        p.character = name;
        p.level = (AccountLevel) level;
        p.timeout = 300; // world ticks
        pendingLogins.insert(std::make_pair(token, p));
    }
}

bool ChatHandler::startListen(enet_uint16 port)
{
    LOG_INFO("Chat handler started:");
    return ConnectionHandler::startListen(port);
}

void ChatHandler::removeOutdatedPending()
{
    ChatPendingLogins::iterator i = pendingLogins.begin(), next;
    while (i != pendingLogins.end())
    {
        next = i; ++next;
        if (--i->second.timeout <= 0) pendingLogins.erase(i);
        i = next;
    }
}

NetComputer *ChatHandler::computerConnected(ENetPeer *peer)
{
    return new ChatClient(peer);
}

void ChatHandler::computerDisconnected(NetComputer *computer)
{
    // Remove user from all channels
    chatChannelManager->removeUserFromAllChannels(((ChatClient*)computer)->characterName);
    ChatPendingClients::iterator i_end = pendingClients.end();
    for (ChatPendingClients::iterator i = pendingClients.begin();
            i != i_end; ++i)
    {
        if (i->second == computer)
        {
            pendingClients.erase(i);
            break;
        }
    }
    delete computer;
}

void ChatHandler::process(enet_uint32 timeout)
{
    ConnectionHandler::process(timeout);
    removeOutdatedPending();
}

void ChatHandler::processMessage(NetComputer *comp, MessageIn &message)
{
    ChatClient &computer = *static_cast< ChatClient * >(comp);
    MessageOut result;

    if (computer.characterName.empty())
    {
        if (message.getId() != PCMSG_CONNECT) return;
        std::string magic_token = message.readString(MAGIC_TOKEN_LENGTH);
        ChatPendingLogins::iterator i = pendingLogins.find(magic_token);
        if (i == pendingLogins.end())
        {
            ChatPendingClients::iterator i_end = pendingClients.end();
            for (ChatPendingClients::iterator i = pendingClients.begin();
                 i != i_end; ++i)
            {
                if (i->second == &computer) return;
            }
            pendingClients.insert(std::make_pair(magic_token, &computer));
            return;
        }

        computer.characterName = i->second.character;
        computer.accountLevel = i->second.level;
        pendingLogins.erase(i);
        result.writeShort(CPMSG_CONNECT_RESPONSE);
        result.writeByte(ERRMSG_OK);
        computer.send(result);
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

    short channel = msg.readShort();

    LOG_DEBUG(client.characterName << " says in channel " << channel << ": "
              << text);

    MessageOut result(CPMSG_PUBMSG);
    result.writeShort(channel);
    result.writeString(client.characterName);
    result.writeString(text);

    // Send the message to the players registered in the channel.
    sendInChannel(channel, result);
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
            chatChannelManager->addUserInChannel(client.characterName,
                                                 channelId);

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

    if (!chatChannelManager->channelExists(channelId))
    {
        // Channel doesn't exist
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (channelId < (signed) MAX_PUBLIC_CHANNELS_RANGE)
    {
        // Public channel

        // Get character based on name
        CharacterPtr character =
            serverHandler->getCharacter(client.characterName);
        std::string channelName = chatChannelManager->getChannelName(channelId);

        if (client.accountLevel == AL_ADMIN || client.accountLevel == AL_GM)
        {
            warnUsersAboutPlayerEventInChat(
                    channelId, "", CHAT_EVENT_LEAVING_PLAYER);
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
        const std::vector<std::string> &userList =
            chatChannelManager->getUserListInChannel(channelId);
        std::vector<std::string>::const_iterator i = userList.begin();
        // If it's actually the private channel's admin
        if (*i == client.characterName)
        {
            // Make every user quit the channel
            warnUsersAboutPlayerEventInChat(
                    channelId, "", CHAT_EVENT_LEAVING_PLAYER);
            if (chatChannelManager->removeChannel(channelId)) {
                reply.writeByte(ERRMSG_OK);
            }
            else
            {
                reply.writeByte(ERRMSG_FAILURE);
            }
        }
        else
        {
            reply.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
        }
    }

    client.send(reply);
}

void
ChatHandler::handleEnterChannelMessage(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_ENTER_CHANNEL_RESPONSE);

    std::string channelName = msg.readString();
    std::string givenPassword = msg.readString();

    short channelId = chatChannelManager->getChannelId(channelName);
    std::string channelPassword =
        chatChannelManager->getChannelPassword(channelId);

    // TODO: b_lindeijer: Currently, the client has to join its guild channels
    //        explicitly by sending 'enter channel' messages. This should be
    //        changed to implicitly joining relevant guild channels right after
    //        login.
    Guild *guild = guildManager->findByName(channelName);

    if (!channelId || !chatChannelManager->channelExists(channelId))
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }
    else if (!channelPassword.empty() && channelPassword != givenPassword)
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
        // In the case of a guild, send user joined message.
        if (guild)
        {
            sendUserJoined(channelId, client.characterName);
        }

        if (chatChannelManager->addUserInChannel(client.characterName,
                                                 channelId))
        {
            reply.writeByte(ERRMSG_OK);
            // The user entered the channel, now give him the channel
            // id, the announcement string and the user list.
            reply.writeShort(channelId);
            reply.writeString(channelName);
            reply.writeString(
                    chatChannelManager->getChannelAnnouncement(channelId));
            const std::vector<std::string> &userList =
                chatChannelManager->getUserListInChannel(channelId);

            for (std::vector<std::string>::const_iterator i = userList.begin(),
                    i_end = userList.end();
                    i != i_end; ++i)
            {
                reply.writeString(*i);
            }
            // Send an CPMSG_UPDATE_CHANNEL to warn other clients a user went
            // in the channel.
            warnUsersAboutPlayerEventInChat(channelId,
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

    if (channelId != 0 && chatChannelManager->channelExists(channelId))
    {
        if (chatChannelManager->removeUserFromChannel(client.characterName,
                                                      channelId))
        {
            reply.writeByte(ERRMSG_OK);
            reply.writeShort(channelId);

            // Send an CPMSG_UPDATE_CHANNEL to warn other clients a user left
            // the channel.
            warnUsersAboutPlayerEventInChat(channelId,
                                            client.characterName,
                                            CHAT_EVENT_LEAVING_PLAYER);

            // TODO: b_lindeijer: Clients aren't supposed to quit guild
            //        channels explicitly, this should rather happen
            //        implicitly. See similar note at handling 'enter channel'
            //        messages.
            std::string channelName =
                chatChannelManager->getChannelName(channelId);

            if (guildManager->doesExist(channelName))
            {
                // Send a user left message
                sendUserLeft(channelId, client.characterName);
            }
        }
        else
        {
            reply.writeByte(ERRMSG_FAILURE);
        }
    }
    else
    {
        reply.writeByte(ERRMSG_INVALID_ARGUMENT);
    }

    client.send(reply);
}

void
ChatHandler::handleListChannelsMessage(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_LIST_CHANNELS_RESPONSE);

    std::list<std::string> publicChannels =
        chatChannelManager->getPublicChannelNames();

    for (std::list<std::string>::iterator i = publicChannels.begin(),
            i_end = publicChannels.end();
            i != i_end; ++i)
    {
        const std::string &name = *i;
        short users = chatChannelManager->getNumberOfChannelUsers(name);
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

    reply.writeString(channelName);

    std::vector<std::string> channelUsers =
        chatChannelManager->getUserListInChannel(
                chatChannelManager->getChannelId(channelName));

    // TODO: b_lindeijer: This method should check whether the channel exists.

    // Add a user at a time
    for (unsigned int i = 0; i < channelUsers.size(); ++i)
    {
        reply.writeString(channelUsers[i]);
    }

    client.send(reply);
}

void
ChatHandler::handleDisconnectMessage(ChatClient &client, MessageIn &msg)
{
    MessageOut reply(CPMSG_DISCONNECT_RESPONSE);
    reply.writeByte(ERRMSG_OK);
    chatChannelManager->removeUserFromAllChannels(client.characterName);
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

void ChatHandler::warnUsersAboutPlayerEventInChat(short channelId,
                                                  const std::string &userName,
                                                  char eventId)
{
    MessageOut result;
    result.writeShort(CPMSG_CHANNEL_EVENT);
    result.writeShort(channelId);
    result.writeByte(eventId);
    result.writeString(userName);
    sendInChannel(channelId, result);
}

void ChatHandler::sendInChannel(short channelId, MessageOut &msg)
{
    // TODO: b_lindeijer: Instead of looping through the channel users for each
    //        connected client, it would be much better to directly associate
    //        the connected clients with the channel.

    const std::vector<std::string> &users =
            chatChannelManager->getUserListInChannel(channelId);

    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i)
    {
        const std::string &name = static_cast<ChatClient*>(*i)->characterName;
        std::vector<std::string>::const_iterator j_end = users.end();

        // If the being is in the channel, send it
        if (std::find(users.begin(), j_end, name) != j_end)
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

void ChatHandler::sendUserJoined(short channelId, const std::string &name)
{
    MessageOut msg(CPMSG_USERJOINED);
    msg.writeShort(channelId);
    msg.writeString(name);
    sendInChannel(channelId, msg);
}

void ChatHandler::sendUserLeft(short channelId, const std::string &name)
{
    MessageOut msg(CPMSG_USERLEFT);
    msg.writeShort(channelId);
    msg.writeString(name);
    sendInChannel(channelId, msg);
}
