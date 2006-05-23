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

#include "chathandler.h"

#include "chatchannelmanager.h"
#include "connectionhandler.h"
#include "messagein.h"
#include "messageout.h"
#include "netcomputer.h"

#include "utils/logger.h"
#include "utils/stringfilter.h"


class ChatClient: public NetComputer
{
    public:
        /**
         * Constructor.
         */
        ChatClient(ChatHandler *, ENetPeer *);

        std::string characterName;
        AccountLevels accountLevel;
};

ChatClient::ChatClient(ChatHandler *handler, ENetPeer *peer):
    NetComputer(handler, peer),
    accountLevel(AL_NORMAL)
{
}

struct ChatPendingLogin
{
    std::string character;
    AccountLevels level;
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
        computer->accountLevel = (AccountLevels)level;
        pendingClients.erase(i);
        MessageOut result;
        result.writeShort(SMSG_CHATSRV_CONNECT_RESPONSE);
        result.writeByte(CSRV_CONNECT_OK);
        computer->send(result.getPacket());
    }
    else
    {
        ChatPendingLogin p;
        p.character = name;
        p.level = (AccountLevels)level;
        p.timeout = 300; // world ticks
        pendingLogins.insert(std::make_pair(token, p));
    }
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
    return new ChatClient(this, peer);
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

    if (computer.characterName.empty()) {
        if (message.getId() != CMSG_CHATSRV_CONNECT) return;
        std::string magic_token = message.readString(32);
        ChatPendingLogins::iterator i = pendingLogins.find(magic_token);
        if (i == pendingLogins.end())
        {
            for (ChatPendingClients::iterator i = pendingClients.begin(), i_end = pendingClients.end();
                 i != i_end; ++i) {
                if (i->second == &computer) return;
            }
            pendingClients.insert(std::make_pair(magic_token, &computer));
            return;
        }
        computer.characterName = i->second.character;
        computer.accountLevel = i->second.level;
        pendingLogins.erase(i);
        MessageOut result;
        result.writeShort(SMSG_CHATSRV_CONNECT_RESPONSE);
        result.writeByte(CSRV_CONNECT_OK);
        computer.send(result.getPacket());
        return;
    }

    switch (message.getId())
    {
        case CMSG_CHAT:
            {
                // chat to people around area
                std::string text = message.readString();
                // If it's slang clean,
                if (stringFilter->filterContent(text))
                {
                    short channel = message.readShort();
                    LOG_INFO("Say: (Channel " << channel << "): " << text, 2);
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

        case CMSG_ANNOUNCE:
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

        case CMSG_PRIVMSG:
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
        case CMSG_REGISTER_CHANNEL:
            {
                MessageOut result;
                result.writeShort(SMSG_REGISTER_CHANNEL_RESPONSE);
                // 0 public, 1 private
                char channelType = message.readByte();
                if (!channelType)
                {
                    if (computer.accountLevel != AL_ADMIN &&
                        computer.accountLevel != AL_GM)
                    {
                        result.writeByte(CHATCNL_CREATE_UNSUFFICIENT_RIGHTS);
                        computer.send(result.getPacket());
                        return;
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
                        result.writeByte(CHATCNL_CREATE_INVALID_NAME);
                        computer.send(result.getPacket());
                        return;
                }
                if (channelAnnouncement.length() > MAX_CHANNEL_ANNOUNCEMENT ||
                    stringFilter->findDoubleQuotes(channelAnnouncement))
                {
                        result.writeByte(CHATCNL_CREATE_INVALID_ANNOUNCEMENT);
                        computer.send(result.getPacket());
                        return;
                }
                if (channelPassword.length() > MAX_CHANNEL_PASSWORD ||
                stringFilter->findDoubleQuotes(channelPassword))
                {
                        result.writeByte(CHATCNL_CREATE_INVALID_PASSWORD);
                        computer.send(result.getPacket());
                        return;
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

                        result.writeByte(CHATCNL_CREATE_OK);
                        result.writeShort(channelId);
                        computer.send(result.getPacket());
                        return;
                    }
                    else
                    {
                        result.writeByte(CHATCNL_CREATE_UNKNOWN);
                        computer.send(result.getPacket());
                        return;
                    }
                }
                else
                {
                    warnPlayerAboutBadWords(computer);
                }
            }
            break;

        case CMSG_UNREGISTER_CHANNEL:
            {
                MessageOut result;
                result.writeShort(SMSG_UNREGISTER_CHANNEL_RESPONSE);

                short channelId = message.readShort();
                if (channelId != 0)
                {
                    // Public channels
                    if (channelId < (signed)MAX_PUBLIC_CHANNELS_RANGE)
                    {
                        if (computer.accountLevel != AL_ADMIN &&
                            computer.accountLevel != AL_GM)
                        {
                            result.writeByte(CHATCNL_DEL_UNSUFFICIENT_RIGHTS);
                        }
                        else
                        { // if the channel actually exist
                            if (chatChannelManager->isChannelRegistered(channelId))
                            {
                                // Make every user quit the channel
                                makeUsersLeaveChannel(channelId);
                                if (chatChannelManager->removeChannel(channelId))
                                    result.writeByte(CHATCNL_DEL_OK);
                                else
                                    result.writeByte(CHATCNL_DEL_UNKNOWN);
                            }
                            else
                            { // Couldn't remove channel because it doesn't exist
                                result.writeByte(CHATCNL_DEL_INVALID_ID);
                            }
                        }
                    }
                    else if (channelId < (signed)MAX_PRIVATE_CHANNELS_RANGE)
                    { // Private channels
                        if (chatChannelManager->isChannelRegistered(channelId))
                        {
                            // We first see if the user is the admin (first user) of the channel
                            std::vector< std::string > const &userList =
                                chatChannelManager->getUserListInChannel(channelId);
                            std::vector< std::string >::const_iterator i = userList.begin();
                            // if it's actually the private channel's admin
                            if (*i == computer.characterName)
                            {
                                // Make every user quit the channel
                                makeUsersLeaveChannel(channelId);
                                if (chatChannelManager->removeChannel(channelId))
                                    result.writeByte(CHATCNL_DEL_OK);
                                else
                                    result.writeByte(CHATCNL_DEL_UNKNOWN);
                            }
                            else
                            {
                                result.writeByte(CHATCNL_DEL_UNSUFFICIENT_RIGHTS);
                            }
                        }
                        else
                        {
                            result.writeByte(CHATCNL_DEL_INVALID_ID);
                        }
                    }
                    else
                    { // Id too high or too low
                        result.writeByte(CHATCNL_DEL_INVALID_ID);
                    }
                }
                else
                {
                    result.writeByte(CHATCNL_DEL_INVALID_ID);
                }
                computer.send(result.getPacket());
            } break;

        case CMSG_ENTER_CHANNEL:
        {
            MessageOut result;
            result.writeShort(SMSG_ENTER_CHANNEL_RESPONSE);
            short channelId = message.readShort();
            std::string givenPassword = message.readString();
            if (channelId != 0 && chatChannelManager->isChannelRegistered(channelId))
            {
                std::string channelPassword = chatChannelManager->getChannelPassword(channelId);
                if (!channelPassword.empty())
                {
                    if (channelPassword != givenPassword)
                    {
                        result.writeByte(CHATCNL_IN_BAD_PASSWORD);
                        computer.send(result.getPacket());
                        break;
                    }
                }
                if (chatChannelManager->addUserInChannel(computer.characterName, channelId))
                {
                    result.writeByte(CHATCNL_IN_OK);
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
                    // Send an SMSG_UPDATE_CHANNEL_RESPONSE to warn other clients a user went
                    // in the channel.
                    warnUsersAboutPlayerEventInChat(channelId, computer.characterName,
                                                    CHATCNL_UPD_NEW_PLAYER);
                }
                else
                {
                    result.writeByte(CHATCNL_IN_UNKNOWN);
                }
            }
            else
            {
                result.writeByte(CHATCNL_IN_INVALID_ID);
            }
            computer.send(result.getPacket());
        }
        break;

        case CMSG_QUIT_CHANNEL:
        {
            MessageOut result;
            result.writeShort(SMSG_QUIT_CHANNEL_RESPONSE);
            short channelId = message.readShort();
            if (channelId != 0 && chatChannelManager->isChannelRegistered(channelId))
            {
                if (chatChannelManager->removeUserFromChannel(computer.characterName, channelId))
                {
                    result.writeByte(CHATCNL_OUT_OK);
                    // Send an SMSG_UPDATE_CHANNEL_RESPONSE to warn other clients a user left
                    // the channel.
                    warnUsersAboutPlayerEventInChat(channelId, computer.characterName,
                                                    CHATCNL_UPD_LEAVING_PLAYER);
                }
                else
                {
                    result.writeByte(CHATCNL_OUT_UNKNOWN);
                }
            }
            else
            {
                result.writeByte(CHATCNL_OUT_INVALID_ID);
            }
            computer.send(result.getPacket());
        }
        break;

        default:
            LOG_INFO("Chat: Invalid message type", 2);
            break;
    }
}

void ChatHandler::handleCommand(ChatClient &computer, std::string const &command)
{
    LOG_INFO("Chat: Received unhandled command: " << command, 2);
    MessageOut result;
    result.writeShort(SMSG_CHAT);
    result.writeByte(CHATCMD_UNHANDLED_COMMAND);
    computer.send(result.getPacket());
}

void ChatHandler::warnPlayerAboutBadWords(ChatClient &computer)
{
    // We could later count if the player is really often unpolite.
    MessageOut result;
    result.writeShort(SMSG_CHAT);
    result.writeByte(CHAT_USING_BAD_WORDS); // The Channel
    computer.send(result.getPacket());

    LOG_INFO(computer.characterName << " says bad words.", 2);
}

void ChatHandler::announce(ChatClient &computer, std::string const &text)
{
    MessageOut result;
    if (computer.accountLevel == AL_ADMIN ||
        computer.accountLevel == AL_GM )
    {
        LOG_INFO("ANNOUNCE: " << text, 0);
        // Send it to every beings.
        result.writeShort(SMSG_ANNOUNCEMENT);
        result.writeString(text);
        sendToEveryone(result);
    }
    else
    {
        result.writeShort(SMSG_CHAT);
        result.writeByte(CHATCMD_UNSUFFICIENT_RIGHTS);
        computer.send(result.getPacket());
        LOG_INFO(computer.characterName <<
            " couldn't make an announcement due to insufficient rights.", 2);
    }
}

void ChatHandler::sayToPlayer(ChatClient &computer, std::string const &playerName, std::string const &text)
{
    MessageOut result;
    LOG_INFO(computer.characterName << " says to " << playerName << ": " << text, 2);
    // Send it to the being if the being exists
    result.writeShort(SMSG_PRIVMSG);
    result.writeString(computer.characterName);
    result.writeString(text);
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i) {
        if (static_cast< ChatClient * >(*i)->characterName == playerName)
        {
            (*i)->send(result.getPacket());
            break;
        }
    }
}

void ChatHandler::sayInChannel(ChatClient &computer, short channel, std::string const &text)
{
    MessageOut result;
    LOG_INFO(computer.characterName << " says in channel " << channel << ": " << text, 2);
    // Send it to every beings in channel
    result.writeShort(SMSG_CHAT_CNL);
    result.writeShort(channel);
    result.writeString(computer.characterName);
    result.writeString(text);
    sendInChannel(channel, result);
}

void ChatHandler::makeUsersLeaveChannel(short channelId)
{
    MessageOut result;
    result.writeShort(SMSG_QUIT_CHANNEL_RESPONSE);
    result.writeByte(CHATCNL_OUT_OK);

    std::vector< std::string > const &users =
            chatChannelManager->getUserListInChannel(channelId);
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i) {
        // If the client is in the channel, send it the 'leave now' packet
        std::vector< std::string >::const_iterator j_end = users.end(),
            j = std::find(users.begin(), j_end, static_cast< ChatClient * >(*i)->characterName);
        if (j != j_end)
        {
            (*i)->send(result.getPacket());
        }
    }
}

void ChatHandler::warnUsersAboutPlayerEventInChat(short channelId,
                                                  std::string const &userName,
                                                  char eventId)
{
    MessageOut result;
    result.writeShort(SMSG_UPDATE_CHANNEL_RESPONSE);
    result.writeByte(eventId);
    result.writeString(userName);

    std::vector< std::string > const &users =
            chatChannelManager->getUserListInChannel(channelId);
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i) {
        // If the client is in the channel, send it the 'eventId' packet
        std::vector< std::string >::const_iterator j_end = users.end(),
            j = std::find(users.begin(), j_end, static_cast< ChatClient * >(*i)->characterName);
        if (j != j_end)
        {
            (*i)->send(result.getPacket());
        }
    }
}

void ChatHandler::sendInChannel(short channelId, MessageOut &msg)
{
    std::vector< std::string > const &users =
            chatChannelManager->getUserListInChannel(channelId);
    for (NetComputers::iterator i = clients.begin(), i_end = clients.end();
         i != i_end; ++i) {
        // If the being is in the channel, send it
        std::vector< std::string >::const_iterator j_end = users.end(),
            j = std::find(users.begin(), j_end, static_cast< ChatClient * >(*i)->characterName);
        if (j != j_end)
        {
            (*i)->send(msg.getPacket());
        }
    }
}
