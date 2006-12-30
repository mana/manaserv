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

#ifndef _TMW_SERVER_GAMEHANDLER_
#define _TMW_SERVER_GAMEHANDLER_

#include "player.h"
#include "net/connectionhandler.hpp"

/**
 * Manages connections to game client.
 */
class GameHandler: public ConnectionHandler
{
    public:
        /**
         * Processes messages and cleans outdated characters.
         */
        void process();

        /**
         * Starts the handler
         */
        bool startListen(enet_uint16 port);

        /**
         * Sends message to the given player.
         */
        void sendTo(Player *, MessageOut &msg);

        /**
         * Kills connection with given player.
         */
        void kill(Player *);

        /**
         * Prepares a server change for given player.
         */
        void prepareServerChange(Player *);

        /**
         * Completes a server change for given player ID.
         */
        void completeServerChange(int id, std::string const &token,
                                  std::string const &address, int port);

    protected:
        NetComputer *computerConnected(ENetPeer *);
        void computerDisconnected(NetComputer *);

        /**
         * Processes messages related to core game events.
         */
        void processMessage(NetComputer *computer, MessageIn &message);
};

extern GameHandler *gameHandler;

#endif
