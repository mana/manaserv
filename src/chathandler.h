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

#include "connectionhandler.h"

class ChatClient;

/**
 * Manages all chat related
 */
class ChatHandler : public ConnectionHandler
{
    public:
        void process();

    protected:
        /**
         * Process chat related messages.
         */
        void processMessage(NetComputer *computer, MessageIn &message);
        NetComputer *computerConnected(ENetPeer *);
        void computerDisconnected(NetComputer *);

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
        void sayToPlayer(ChatClient &computer, std::string const &playerName, std::string const &text);

        /**
         * Say something in a specific channel.
         */
        void sayInChannel(ChatClient &computer, short channel, std::string const &);

        /**
         * Send packet to every client in a registered channel.
         */
        void sendInChannel(short channelId, MessageOut &);

        /**
         * Tell a list of user about an event in a chatchannel about a player.
         */
        void warnUsersAboutPlayerEventInChat(short channelId, std::string const &userName, char eventId);

        void removeOutdatedPending();
};

/**
 * Register future client attempt. Temporary until physical server split.
 */
void registerChatClient(std::string const &, std::string const &, int);

#endif
