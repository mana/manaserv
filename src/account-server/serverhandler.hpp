/*
 *  The Mana World Server
 *  Copyright 2006 The Mana World Development Team
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

#ifndef _TMWSERV_SERVERHANDLER_H_
#define _TMWSERV_SERVERHANDLER_H_

#include <iosfwd>
#include <string>

#include "net/connectionhandler.hpp"

class AccountClient;
class Character;
struct GameServer;

/**
 * Manages communications with all the game servers. This class also keeps
 * track of the maps each game server supports.
 */
class ServerHandler: public ConnectionHandler
{
    public:
        /**
         * Starts the handler on the given port.
         */
        bool startListen(enet_uint16 port);

        /**
         * Returns the information a client needs to connect to the game server
         * corresponding to the given map ID.
         */
        bool getGameServerFromMap(int, std::string &address, int &port) const;

        /**
         * Sends a magic token and character data to the relevant game server.
         */
        void registerGameClient(std::string const &, Character *);

// There is no rationale for having a character name, but not its ID.
#if 0
        /**
         * Get character (temp used by chat server).
         */
        CharacterPtr getCharacter(const std::string &name);
#endif

        /**
         * Make client join the specified guild channel
         */
        void enterChannel(const std::string &guildName, Character *player);

        /**
         * Dumps per-server statistics into given stream
         */
        void dumpStatistics(std::ostream &) const;

    protected:
        /**
         * Processes server messages.
         */
        void processMessage(NetComputer *computer, MessageIn &message);

        /**
         * Called when a game server connects. Initializes a simple NetComputer
         * as these connections are stateless.
         */
        NetComputer *computerConnected(ENetPeer *peer);

        /**
         * Called when a game server disconnects.
         */
        void computerDisconnected(NetComputer *comp);

    private:
        
        /**
         * Returns the information a client needs to connect to the game server
         * corresponding to the given map ID.
         */
        GameServer *getGameServerFromMap(int) const;

#if 0
        /**
         * Send invite to user
         */
        void sendInvite(const std::string &invitedName, const std::string &inviterName,
                        const std::string &guildName);
#endif
};

extern ServerHandler *serverHandler;

#endif
