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
 * @todo <b>b_lindeijer:</b> Extend this class with handling of team chat once
 *       teams are implemented.
 */
class ChatHandler : public ConnectionHandler
{

        /**
         * Data needed for initializing a ChatClient.
         */
        struct Pending
        {
            std::string character;
            unsigned char level;
        };

    public:

        ChatHandler();

        /**
         * Start the handler.
         */
        bool startListen(enet_uint16 port);

        /**
         * Tell a list of users about an event in a chatchannel about a player.
         *
         * @param channel the channel to send the message in, must not be NULL
         * @param userName the name of the player the event applies to
         */
        void warnUsersAboutPlayerEventInChat(ChatChannel *channel,
                                             const std::string &userName,
                                             char eventId);

        /**
         * Send chat and guild info to chat client, so that they can join the
         * correct channels.
         */
        void sendGuildEnterChannel(const MessageOut &msg,
                                   const std::string &name);

        /**
         * Send guild invite.
         */
        void sendGuildInvite(const std::string &invitedName,
                             const std::string &inviterName,
                             const std::string &guildName);

        /**
         * Called by TokenCollector when a client wrongly connected.
         */
        void deletePendingClient(ChatClient *);

        /**
         * Called by TokenCollector when a client failed to connect.
         */
        void deletePendingConnect(Pending *);

        /**
         * Called by TokenCollector when a client succesfully connected.
         */
        void tokenMatched(ChatClient *, Pending *);

    protected:
        /**
         * Process chat related messages.
         */
        void processMessage(NetComputer *computer, MessageIn &message);

        /**
         * Returns a ChatClient instance.
         */
        NetComputer *computerConnected(ENetPeer *);

        /**
         * Cleans up after the disconnected client.
         */
        void computerDisconnected(NetComputer *);

        /**
         * Send messages for each guild the character belongs to.
         */
        void sendGuildRejoin(ChatClient &computer);

    private:
        /**
         * Deal with command messages.
         */
        void handleCommand(ChatClient &client, const std::string &command);

        void
        handleChatMessage(ChatClient &client, MessageIn &msg);

        void
        handleAnnounceMessage(ChatClient &client, MessageIn &msg);

        void
        handlePrivMsgMessage(ChatClient &client, MessageIn &msg);

        void
        handleRegisterChannelMessage(ChatClient &client, MessageIn &msg);

        void
        handleUnregisterChannelMessage(ChatClient &client, MessageIn &msg);

        void
        handleEnterChannelMessage(ChatClient &client, MessageIn &msg);

        void
        handleQuitChannelMessage(ChatClient &client, MessageIn &msg);

        void
        handleListChannelsMessage(ChatClient &client, MessageIn &msg);

        void
        handleListChannelUsersMessage(ChatClient &client, MessageIn &msg);

        void
        handleDisconnectMessage(ChatClient &client, MessageIn &msg);

        /**
         * Tell the player to be more polite.
         */
        void warnPlayerAboutBadWords(ChatClient &computer);

        /**
         * Say something private to a player.
         */
        void sayToPlayer(ChatClient &computer, const std::string &playerName,
                         const std::string &text);

        /**
         * Sends a message to every client in a registered channel.
         *
         * @param channel the channel to send the message in, must not be NULL
         * @param msg     the message to be sent
         */
        void sendInChannel(ChatChannel *channel, MessageOut &msg);

        /**
         * Send user joined message.
         *
         * @param channel the channel to send the message in, must not be NULL
         * @param name    the name of the user who joined
         */
        void sendUserJoined(ChatChannel *channel, const std::string &name);

        /**
         * Send user left message.
         *
         * @param channel the channel to send the message in, must not be NULL
         * @param name    the name of the user who left
         */
        void sendUserLeft(ChatChannel *channel, const std::string &name);

        /**
         * Container for pending clients and pending connections.
         */
        TokenCollector<ChatHandler, ChatClient *, Pending *> mTokenCollector;
        friend void registerChatClient(const std::string &, const std::string &, int);

};

/**
 * Register future client attempt. Temporary until physical server split.
 */
void registerChatClient(const std::string &, const std::string &, int);

extern ChatHandler *chatHandler;

#endif
