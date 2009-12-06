/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
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

#ifndef CHATHANDLER_H
#define CHATHANDLER_H

#include <map>
#include <string>
#include <vector>

#include "net/connectionhandler.hpp"
#include "utils/tokencollector.hpp"

class ChatChannel;
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
    private:
        /**
         * Data needed for initializing a ChatClient.
         */
        struct Pending
        {
            std::string character;
            unsigned char level;
        };

        struct PartyInvite
        {
            std::string mInviter;
            std::string mInvited;
            int mPartyId;
        };

        std::map<std::string, ChatClient*> mPlayerMap;
        std::vector<PartyInvite> mPartyInvitedUsers;

    public:
        ChatHandler();

        /**
         * Start the handler.
         */
        bool startListen(enet_uint16 port, const std::string &host);

        /**
         * Tell a list of users about an event in a chatchannel.
         *
         * @param channel the channel to send the message in, must not be NULL
         * @param info information pertaining to the event
         */
        void warnUsersAboutPlayerEventInChat(ChatChannel *channel,
                                             const std::string &info,
                                             char eventId);

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

        /**
         * Send information about a change in the guild list to guild members.
         */
        void sendGuildListUpdate(const std::string &guildName,
                                 const std::string &characterName,
                                 char eventId);

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

    private:
        /**
         * Deal with command messages.
         */
        void handleCommand(ChatClient &client, const std::string &command);

        /**
         * Deal with Chat messages.
         */
        void handleChatMessage(ChatClient &client, MessageIn &msg);

        /**
         * Deal with Announcement messages.
         */
        void handleAnnounceMessage(ChatClient &client, MessageIn &msg);

        /**
         * Deal with Private messages.
         */
        void handlePrivMsgMessage(ChatClient &client, MessageIn &msg);

        /**
         * Deal with Who messages.
         */
        void handleWhoMessage(ChatClient &client);

        /**
         * Deal with player entering channel.
         */
        void handleEnterChannelMessage(ChatClient &client, MessageIn &msg);

        /**
         * Deal with player changing mode.
         */
        void handleModeChangeMessage(ChatClient &client, MessageIn &msg);

        /**
         * Deal with player kicking other player from channel.
         */
        void handleKickUserMessage(ChatClient &client, MessageIn &msg);

        /**
         * Deal with player leaving channel.
         */
        void handleQuitChannelMessage(ChatClient &client, MessageIn &msg);

        /**
         * Deal with listing all accessible channels.
         */
        void handleListChannelsMessage(ChatClient &client, MessageIn &msg);

        /**
         * Deal with listing all channel users in a channel.
         */
        void handleListChannelUsersMessage(ChatClient &client, MessageIn &msg);

        /**
         * Deal with changing a channel's topic
         */
        void handleTopicChange(ChatClient &client, MessageIn &msg);

        /**
         * Deal with disconnection.
         */
        void handleDisconnectMessage(ChatClient &client, MessageIn &msg);

        /**
         * Deal with creating a guild.
         */
        void handleGuildCreation(ChatClient &client, MessageIn &msg);

        /**
         * Deal with inviting a player to a guild.
         */
        void handleGuildInvitation(ChatClient &client, MessageIn &msg);

        /**
         * Deal with accepting an invite to join a guild.
         */
        void handleGuildAcceptInvite(ChatClient &client, MessageIn &msg);

        /**
         * Deal with returning all the guild members of a guild.
         */
        void handleGuildRetrieveMembers(ChatClient &client, MessageIn &msg);

        /**
         * Deal with level change of member
         */
        void handleGuildMemberLevelChange(ChatClient &client, MessageIn &msg);

        /**
         * Deal with kicking a member
         */
        void handleGuildMemberKick(ChatClient &client, MessageIn &msg);

        /**
         * Deal with leaving a guild.
         */
        void handleGuildQuit(ChatClient &client, MessageIn &msg);

        /**
         * Deal with a player joining a party.
         * @return Returns whether player successfully joined the party
         */
        bool handlePartyJoin(const std::string &invited,
                             const std::string &inviter);

        /**
         * Deal with inviting player to a party
         */
        void handlePartyInvite(ChatClient &client, MessageIn &msg);

        /**
         * Deal with accepting an invite to join a party
         */
        void handlePartyAcceptInvite(ChatClient &client, MessageIn &msg);

        /**
         * Deal with leaving a party.
         */
        void handlePartyQuit(ChatClient &client);

        /**
         * Tell user the invite was rejected
         */
        void handlePartyRejection(ChatClient &client, MessageIn &msg);

        /**
         * Remove user from party
         */
        void removeUserFromParty(ChatClient &client);

        /**
         * Send new member info to party members.
         */
        void sendPartyMemberInfo(ChatClient &client, MessageIn &msg);

        /**
         * Tell all the party members a member has left
         */
        void informPartyMemberQuit(ChatClient &client);

        /**
         * Tell all the party members a member has joined
         */
        void informPartyMemberJoined(ChatClient &client);

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
         * Retrieves the guild channel or creates one automatically
         * Automatically makes client join it
         * @param The name of the guild (and therefore the channel)
         * @param The client to join the channel
         * @return Returns the channel joined
         */
        ChatChannel *joinGuildChannel(const std::string &name,
                                      ChatClient &client);

        /**
         * Returns ChatClient from the Player Map
         * @param The name of the character
         * @return The Chat Client
         */
        ChatClient *getClient(const std::string &name);

        /**
         * Set the topic of a guild channel
         */
        void guildChannelTopicChange(ChatChannel *channel, int playerId,
                                     const std::string &topic);

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
