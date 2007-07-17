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

#ifndef _TMWSERV_CHATHANDLER_H_
#define _TMWSERV_CHATHANDLER_H_

#include <iosfwd>

#include "net/connectionhandler.hpp"

class ChatClient;

/**
 * Manages chat related things like private messaging, chat channel handling
 * as well as guild chat. The only form of chat not handled by this server is
 * local chat, which is handled by the game server.
 *
 * TODO: Extend with handling of team chat once teams are implemented.
 */
class ChatHandler : public ConnectionHandler
{
    public:
        /**
         * Overridden from ConnectionHandler to also clean connected clients
         * that haven't sent in a magic token.
         *
         * @see ConnectionHandler::process
         */
        void process(enet_uint32 timeout = 0);

        /**
         * Start the handler.
         */
        bool
        startListen(enet_uint16 port);

        /**
         * Tell a list of users about an event in a chatchannel about a player.
         */
        void warnUsersAboutPlayerEventInChat(short channelId,
                                             std::string const &userName,
                                             char eventId);

        /**
         * Send Chat and Guild Info to chat client, so that they can
         * join the correct channels.
         */
        void sendGuildEnterChannel(const MessageOut &msg,
                                   const std::string &name);

        /**
         * Send guild invite.
         */
        void sendGuildInvite(const std::string &invitedName,
                             const std::string &inviterName,
                             const std::string &guildName);

    protected:
        /**
         * Process chat related messages.
         */
        void processMessage(NetComputer *computer, MessageIn &message);
        NetComputer *computerConnected(ENetPeer *);
        void computerDisconnected(NetComputer *);

        /**
         * Send messages for each guild the character belongs to.
         */
        void sendGuildRejoin(ChatClient &computer);

    private:
        /**
         * Deal with command messages.
         */
        void handleCommand(ChatClient &computer, std::string const &command);

        /**
         * Tell the player to be more polite.
         */
        void warnPlayerAboutBadWords(ChatClient &computer);

        /**
         * Announce a message to every being in the default channel.
         */
        void announce(ChatClient &computer, std::string const &text);

        /**
         * Say something private to a player.
         */
        void sayToPlayer(ChatClient &computer, std::string const &playerName,
                         std::string const &text);

        /**
         * Say something in a specific channel.
         */
        void sayInChannel(ChatClient &computer, short channel,
                          std::string const &);

        /**
         * Send packet to every client in a registered channel.
         */
        void sendInChannel(short channelId, MessageOut &);

        /**
         * Removes outdated pending logins. These are connected clients that
         * still haven't sent in their magic token.
         */
        void removeOutdatedPending();

        /**
         * Send user joined message.
         */
        void sendUserJoined(short channelId, const std::string &name);

        /**
         * Send user left message.
         */
        void sendUserLeft(short channelId, const std::string &name);
};

/**
 * Register future client attempt. Temporary until physical server split.
 */
void registerChatClient(std::string const &, std::string const &, int);

extern ChatHandler *chatHandler;

#endif
