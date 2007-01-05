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

#include "defines.h"
#include "chat-server/chatchannelmanager.hpp"
#include "chat-server/chathandler.hpp"
#include "net/connectionhandler.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "net/netcomputer.hpp"
#include "utils/logger.h"
#include "utils/stringfilter.h"


class ChatClient: public NetComputer
{
    public:
        /**
         * Constructor.
         */
        ChatClient(ENetPeer *peer);

        std::string characterName;
        AccountLevel accountLevel;
};

ChatClient::ChatClient(ENetPeer *peer):
    NetComputer(peer),
    accountLevel(AL_NORMAL)
{
}

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

void registerChatClient(std::string const &token, std::string const &name, int level)
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

bool
ChatHandler::startListen(enet_uint16 port)
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
    for (ChatPendingClients::iterator i = pendingClients.begin(), i_end = pendingClients.end();
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

void ChatHandler::process()
{
    ConnectionHandler::process();
    removeOutdatedPending();
}

void ChatHandler::processMessage(NetComputer *comp, MessageIn &message)
{
    ChatClient &computer = *static_cast< ChatClient * >(comp);
    MessageOut result;

    if (computer.characterName.empty()) {
        if (message.getId() != PCMSG_CONNECT) return;
        std::string magic_token = message.readString(32);
        ChatPendingLogins::iterator i = pendingLogins.find(magic_token);
        if (i == pendingLogins.end())
        {
            for (ChatPendingClients::iterator i = pendingClients.begin(), i_end = pendingClients.end();
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
        return;
    }

    switch (message.getId())
    {
        case PCMSG_CHAT:
            {
                // chat to people around area
                std::string text = message.readString();
                // If it's slang clean,
                if (stringFilter->filterContent(text))
                {
                    short channel = message.readShort();
                    LOG_DEBUG("Say: (Channel " << channel << "): " << text);
                    if ( channel == 0 ) // Let's say that is the default channel for now.
                    {
                        if ( text.substr(0, 1) == "@" || text.substr(0, 1) == "#" || text.substr(0, 1) == "/" )
                        {
                            // The message is a command. Deal with it.
                            handleCommand(computer, text);
                        }
                    }
                    else
                    {
                        // We send the message to the players registered in the channel.
                        sayInChannel(computer, channel, text);
                    }
                }
                else
                {
                    warnPlayerAboutBadWords(computer);
                }
            }
            break;

        case PCMSG_ANNOUNCE:
            {
                std::string text = message.readString();
                // If it's slang's free.
                if (stringFilter->filterContent(text))
                {
                    // We send the message to every players in the default channel
                    // as it is an annouce.
                    announce(computer, text);
                }
                else
                {
                    warnPlayerAboutBadWords(computer);
                }
            }
            break;

        case PCMSG_PRIVMSG:
            {
                std::string user = message.readString();
                std::string text = message.readString();
                if (stringFilter->filterContent(text))
                {
                    // We seek the player to whom the message is told
                    // and send it to her/him.
                    sayToPlayer(computer, user, text);
                }
                else
                {
                    warnPlayerAboutBadWords(computer);
                }
            } break;

        // Channels handling
        // =================
        case PCMSG_REGISTER_CHANNEL:
            {
                result.writeShort(CPMSG_REGISTER_CHANNEL_RESPONSE);
                // 0 public, 1 private
                char channelType = message.readByte();
                if (!channelType)
                {
                    if (computer.accountLevel != AL_ADMIN &&
                        computer.accountLevel != AL_GM)
                    {
                        result.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
                        break;
                    }
                }
                std::string channelName = message.readString();
                std::string channelAnnouncement = message.readString();
                std::string channelPassword = message.readString();
                // Checking datas
                // Seeking double-quotes in strings
                if (channelName.empty() || channelName.length() > MAX_CHANNEL_NAME ||
                stringFilter->findDoubleQuotes(channelName))
                {
                        result.writeByte(ERRMSG_INVALID_ARGUMENT);
                        break;
                }
                if (channelAnnouncement.length() > MAX_CHANNEL_ANNOUNCEMENT ||
                    stringFilter->findDoubleQuotes(channelAnnouncement))
                {
                        result.writeByte(ERRMSG_INVALID_ARGUMENT);
                        break;
                }
                if (channelPassword.length() > MAX_CHANNEL_PASSWORD ||
                stringFilter->findDoubleQuotes(channelPassword))
                {
                        result.writeByte(ERRMSG_INVALID_ARGUMENT);
                        break;
                }

                // If it's slang's free.
                if (stringFilter->filterContent(channelName) &&
                stringFilter->filterContent(channelAnnouncement))
                {
                    // We attempt to create a new channel
                    short channelId;
                    if (channelType)
                        channelId = chatChannelManager->registerPrivateChannel(channelName,
                                                                            channelAnnouncement,
                                                                            channelPassword);
                    else
                        channelId = chatChannelManager->registerPublicChannel(channelName,
                                                                            channelAnnouncement,
                                                                            channelPassword);
                    if (channelId != 0)
                    {
                        // We add the player as admin of this channel as he created it.
                        // The user registering a private channel is the only one to be able
                        // to update the password and the announcement in it and also to remove it.
                        chatChannelManager->addUserInChannel(computer.characterName, channelId);

                        result.writeByte(ERRMSG_OK);
                        result.writeShort(channelId);
                        break;
                    }
                    else
                    {
                        result.writeByte(ERRMSG_FAILURE);
                        break;
                    }
                }
                else
                {
                    warnPlayerAboutBadWords(computer);
                }
            }
            break;

        case PCMSG_UNREGISTER_CHANNEL:
            {
                result.writeShort(CPMSG_UNREGISTER_CHANNEL_RESPONSE);

                short channelId = message.readShort();
                if (!chatChannelManager->isChannelRegistered(channelId))
                {
                    result.writeByte(ERRMSG_INVALID_ARGUMENT);
                }
                else if (channelId < (signed)MAX_PUBLIC_CHANNELS_RANGE)
                { // Public channel
                    if (computer.accountLevel == AL_ADMIN || computer.accountLevel == AL_GM)
                    {
                        warnUsersAboutPlayerEventInChat(channelId, "", CHAT_EVENT_LEAVING_PLAYER);
                        if (chatChannelManager->removeChannel(channelId))
                            result.writeByte(ERRMSG_OK);
                        else
                            result.writeByte(ERRMSG_FAILURE);
                    }
                    else
                    {
                        result.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
                    }
                }
                else
                { // Private channel
                    // We first see if the user is the admin (first user) of the channel
                    std::vector< std::string > const &userList =
                        chatChannelManager->getUserListInChannel(channelId);
                    std::vector< std::string >::const_iterator i = userList.begin();
                    // if it's actually the private channel's admin
                    if (*i == computer.characterName)
                    {
                        // Make every user quit the channel
                        warnUsersAboutPlayerEventInChat(channelId, "", CHAT_EVENT_LEAVING_PLAYER);
                        if (chatChannelManager->removeChannel(channelId))
                            result.writeByte(ERRMSG_OK);
                        else
                            result.writeByte(ERRMSG_FAILURE);
                    }
                    else
                    {
                        result.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
                    }
                }
            } break;

        case PCMSG_ENTER_CHANNEL:
        {
            result.writeShort(CPMSG_ENTER_CHANNEL_RESPONSE);
            short channelId = message.readShort();
            std::string givenPassword = message.readString();
            if (channelId != 0 && chatChannelManager->isChannelRegistered(channelId))
            {
                std::string channelPassword = chatChannelManager->getChannelPassword(channelId);
                if (!channelPassword.empty())
                {
                    if (channelPassword != givenPassword)
                    {
                        result.writeByte(ERRMSG_INVALID_ARGUMENT);
                        break;
                    }
                }
                if (chatChannelManager->addUserInChannel(computer.characterName, channelId))
                {
                    result.writeByte(ERRMSG_OK);
                    // The user entered the channel, now give him the announcement string
                    // and the user list.
                    result.writeString(chatChannelManager->getChannelAnnouncement(channelId));
                    std::vector< std::string > const &userList =
                    chatChannelManager->getUserListInChannel(channelId);
                    result.writeShort(userList.size());
                    for (std::vector< std::string >::const_iterator i = userList.begin(), i_end = userList.end();
                         i != i_end; ++i) {
                        result.writeString(*i);
                    }
                    // Send an CPMSG_UPDATE_CHANNEL to warn other clients a user went
                    // in the channel.
                    warnUsersAboutPlayerEventInChat(channelId, computer.characterName,
                                                    CHAT_EVENT_NEW_PLAYER);
                }
                else
                {
                    result.writeByte(ERRMSG_FAILURE);
                }
            }
            else
            {
                result.writeByte(ERRMSG_INVALID_ARGUMENT);
            }
        }
        break;

        case PCMSG_QUIT_CHANNEL:
        {
            result.writeShort(CPMSG_QUIT_CHANNEL_RESPONSE);
            short channelId = message.readShort();
            if (channelId != 0 && chatChannelManager->isChannelRegistered(channelId))
            {
                if (chatChannelManager->removeUserFromChannel(computer.characterName, channelId))
                {
                    result.writeByte(ERRMSG_OK);
                    // Send an CPMSG_UPDATE_CHANNEL to warn other clients a user left
                    // the channel.
                    warnUsersAboutPlayerEventInChat(channelId, computer.characterName,
                                                    CHAT_EVENT_LEAVING_PLAYER);
                }
                else
                {
                    result.writeByte(ERRMSG_FAILURE);
                }
            }
            else
            {
                result.writeByte(ERRMSG_INVALID_ARGUMENT);
            }
        }
        break;

        default:
            LOG_WARN("Invalid message type");
            result.writeShort(XXMSG_INVALID);
            break;
    }

    if (result.getLength() > 0)
        computer.send(result);
}

void ChatHandler::handleCommand(ChatClient &computer, std::string const &command)
{
    LOG_INFO("Chat: Received unhandled command: " << command);
    MessageOut result;
    result.writeShort(CPMSG_ERROR);
    result.writeByte(CHAT_UNHANDLED_COMMAND);
    computer.send(result);
}

void ChatHandler::warnPlayerAboutBadWords(ChatClient &computer)
{
    // We could later count if the player is really often unpolite.
    MessageOut result;
    result.writeShort(CPMSG_ERROR);
    result.writeByte(CHAT_USING_BAD_WORDS); // The Channel
    computer.send(result);

    LOG_INFO(computer.characterName << " says bad words.");
}

void ChatHandler::announce(ChatClient &computer, std::string const &text)
{
    MessageOut result;
    if (computer.accountLevel == AL_ADMIN ||
        computer.accountLevel == AL_GM )
    {
        LOG_INFO("ANNOUNCE: " << text);
        // Send it to every beings.
        result.writeShort(CPMSG_ANNOUNCEMENT);
        result.writeString(text);
        sendToEveryone(result);
    }
    else
    {
        result.writeShort(CPMSG_ERROR);
        result.writeByte(ERRMSG_INSUFFICIENT_RIGHTS);
        computer.send(result);
        LOG_INFO(computer.characterName <<
            " couldn't make an announcement due to insufficient rights.");
    }
}

void ChatHandler::sayToPlayer(ChatClient &computer, std::string const &playerName, std::string const &text)
{
    MessageOut result;
    LOG_DEBUG(computer.characterName << " says to " << playerName << ": " << text);
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

void ChatHandler::sayInChannel(ChatClient &computer, short channel, std::string const &text)
{
    MessageOut result;
    LOG_DEBUG(computer.characterName << " says in channel " << channel << ": " << text);
    // Send it to every beings in channel
    result.writeShort(CPMSG_PUBMSG);
    result.writeShort(channel);
    result.writeString(computer.characterName);
    result.writeString(text);
    sendInChannel(channel, result);
}

void ChatHandler::warnUsersAboutPlayerEventInChat(short channelId,
                                                  std::string const &userName,
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
    std::vector< std::string > const &users =
            chatChannelManager->getUserListInChannel(channelId);
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i)
    {
        // If the being is in the channel, send it
        std::vector< std::string >::const_iterator j_end = users.end(),
            j = std::find(users.begin(), j_end, static_cast< ChatClient * >(*i)->characterName);
        if (j != j_end)
        {
            (*i)->send(msg);
        }
    }
}
