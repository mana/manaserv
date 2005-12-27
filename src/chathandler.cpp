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

#include <cctype>
#include "chathandler.h"
#include "state.h"
#include "being.h"
#include "defines.h"
#include "utils/logger.h"
#include "utils/slangsfilter.h"

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
                if (tmwserv::utils::filterContent(text))
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
                            // By 'around', let's say 10 tiles square wide for now.
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
                if (tmwserv::utils::filterContent(text))
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
                if (tmwserv::utils::filterContent(text))
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

        default:
            LOG_INFO("Chat: Invalid message type", 2)
            break;
    }
}

void ChatHandler::handleCommand(NetComputer &computer, std::string command)
{
    LOG_INFO("Chat: Recieved unhandled command: " << command, 2)
    MessageOut result;
    result.writeShort(SMSG_CHAT);
    result.writeShort(0); // The Channel
    result.writeString("SERVER: Unknown or unhandled command.");
    computer.send(result.getPacket());
}

void ChatHandler::warnPlayerAboutBadWords(NetComputer &computer)
{
    // We could later count if the player is really often unpolite.
    MessageOut result;
    result.writeShort(SMSG_CHAT);
    result.writeShort(0); // The Channel
    result.writeString("SERVER: Take care not to use bad words when you speak...");
    computer.send(result.getPacket());

    LOG_INFO(computer.getCharacter()->getName() << " says bad words.", 2)
}

void ChatHandler::announce(NetComputer &computer, std::string text)
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
        result.writeShort(SMSG_SYSTEM);
        result.writeString("Cannot make announcements. You have not enough rights.");
        computer.send(result.getPacket());
        LOG_INFO(computer.getCharacter()->getName() <<
            " couldn't make an announcement due to insufficient rights.", 2)
    }
}

void ChatHandler::sayAround(NetComputer &computer, std::string text)
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

void ChatHandler::sayToPlayer(NetComputer &computer, std::string playerName, std::string text)
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

void ChatHandler::sayInChannel(NetComputer &computer, short channel, std::string text)
{
    MessageOut result;
    LOG_INFO( computer.getCharacter()->getName() << " says in channel " << channel
            << ": " << text, 2)
    // TODO: Send it to every beings in channel
    result.writeShort(SMSG_CHAT);
    result.writeShort(channel);
    std::string say = computer.getCharacter()->getName();
    say += ": ";
    say += text;
    result.writeString(say);
}
