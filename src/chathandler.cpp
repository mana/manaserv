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
#include "state.h"
#include "being.h"
#include "defines.h"
#include "utils/logger.h"
#include "utils/stringfilter.h"
#include "chatchannelmanager.h"

void ChatHandler::receiveMessage(NetComputer &computer, MessageIn &message)
{
    // If not logged in...
    if (computer.getAccount().get() == NULL)
    {
        LOG_INFO("Not logged in, can't chat...", 2)
        MessageOut result;
        result.writeShort(SMSG_CHAT);
        result.writeByte(CHAT_NOLOGIN);
        computer.send(result.getPacket());
        return;
    }
    else
    {
        // If no character selected yet...
        if (computer.getCharacter().get() == NULL)
        {
            MessageOut result;
            result.writeShort(SMSG_CHAT);
            result.writeByte(CHAT_NO_CHARACTER_SELECTED);
            computer.send(result.getPacket());
            LOG_INFO("No character selected. Can't chat...", 2)
            return; // character not selected
        }
    }

    switch (message.getId())
    {
        case CMSG_SAY:
            {
                // chat to people around area
                std::string text = message.readString();
                // If it's slang clean,
                if (stringFilter->filterContent(text))
                {
                    short channel = message.readShort();
                    LOG_INFO("Say: (Channel " << channel << "): " << text, 2)
                    if ( channel == 0 ) // Let's say that is the default channel for now.
                    {
                        if ( text.substr(0, 1) == "@" || text.substr(0, 1) == "#" || text.substr(0, 1) == "/" )
                        {
                            // The message is a command. Deal with it.
                            handleCommand(computer, text);
                        }
                        else
                        {
                            // The default channel (0) is when the character speaks
                            // to the characters around him in the map.
                            // We, then, look for every characters around him and
                            // send the message to them.
                            // By 'around', let's say AROUND_AREA_IN_TILES tiles square wide.
                            sayAround(computer, text);
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
                    if (computer.getAccount()->getLevel() != (AccountLevels)AL_ADMIN &&
                        computer.getAccount()->getLevel() != (AccountLevels)AL_GM)
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
                if (channelName == "" || channelName.length() > MAX_CHANNEL_NAME ||
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
                        chatChannelManager->addUserInChannel(computer.getCharacter(), channelId);

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
                        if (computer.getAccount()->getLevel() != (AccountLevels)AL_ADMIN &&
                        computer.getAccount()->getLevel() != (AccountLevels)AL_GM)
                        {
                            result.writeByte(CHATCNL_DEL_UNSUFFICIENT_RIGHTS);
                        }
                        else
                        { // if the channel actually exist
                            if (chatChannelManager->isChannelRegistered(channelId))
                            {
                                // Make every user quit the channel
                                connectionHandler->makeUsersLeaveChannel(channelId);
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
                            std::vector<tmwserv::BeingPtr> userList =
                                chatChannelManager->getUserListInChannel(channelId);
                            std::vector<tmwserv::BeingPtr>::const_iterator i = userList.begin();
                            // if it's actually the private channel's admin
                            if ( (*i).get() == computer.getCharacter().get() )
                            {
                                // Make every user quit the channel
                                connectionHandler->makeUsersLeaveChannel(channelId);
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



        default:
            LOG_INFO("Chat: Invalid message type", 2)
            break;
    }
}

void ChatHandler::handleCommand(NetComputer &computer, const std::string& command)
{
    LOG_INFO("Chat: Received unhandled command: " << command, 2)
    MessageOut result;
    result.writeShort(SMSG_CHAT);
    result.writeByte(CHATCMD_UNHANDLED_COMMAND);
    computer.send(result.getPacket());
}

void ChatHandler::warnPlayerAboutBadWords(NetComputer &computer)
{
    // We could later count if the player is really often unpolite.
    MessageOut result;
    result.writeShort(SMSG_CHAT);
    result.writeByte(CHAT_USING_BAD_WORDS); // The Channel
    computer.send(result.getPacket());

    LOG_INFO(computer.getCharacter()->getName() << " says bad words.", 2)
}

void ChatHandler::announce(NetComputer &computer, const std::string& text)
{
    MessageOut result;
    if ( computer.getAccount()->getLevel() == (AccountLevels)AL_ADMIN ||
    computer.getAccount()->getLevel() == (AccountLevels)AL_GM )
    {
        LOG_INFO("ANNOUNCE: " << text, 0)
        // Send it to every beings.
        result.writeShort(SMSG_ANNOUNCEMENT);
        result.writeString(text);
        connectionHandler->sendToEveryone(result);
    }
    else
    {
        result.writeShort(SMSG_CHAT);
        result.writeByte(CHATCMD_UNSUFFICIENT_RIGHTS);
        computer.send(result.getPacket());
        LOG_INFO(computer.getCharacter()->getName() <<
            " couldn't make an announcement due to insufficient rights.", 2)
    }
}

void ChatHandler::sayAround(NetComputer &computer, const std::string& text)
{
    MessageOut result;
    LOG_INFO( computer.getCharacter()->getName() << " says: " << text, 2)
    // Send it to every beings around
    result.writeShort(SMSG_CHAT);
    result.writeShort(0); // The default channel
    std::string say = computer.getCharacter()->getName();
    say += ": ";
    say += text;
    result.writeString(say);
    connectionHandler->sendAround(computer.getCharacter(), result);
}

void ChatHandler::sayToPlayer(NetComputer &computer, const std::string& playerName, const std::string& text)
{
    MessageOut result;
    LOG_INFO( computer.getCharacter()->getName() << " says to " << playerName
            << ": " << text, 2)
    // Send it to the being if the being exists
    result.writeShort(SMSG_PRIVMSG);
    std::string say = computer.getCharacter()->getName();
    say += ": ";
    say += text;
    result.writeString(say);
    connectionHandler->sendTo(playerName, result);
}

void ChatHandler::sayInChannel(NetComputer &computer, short channel, const std::string& text)
{
    MessageOut result;
    LOG_INFO( computer.getCharacter()->getName() << " says in channel " << channel
            << ": " << text, 2)
    // Send it to every beings in channel
    result.writeShort(SMSG_CHAT);
    result.writeShort(channel);
    std::string say = computer.getCharacter()->getName();
    say += ": ";
    say += text;
    result.writeString(say);
    connectionHandler->sendInChannel(channel, result);

}
