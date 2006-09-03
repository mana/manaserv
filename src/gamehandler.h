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

#include "account.h"
#include "connectionhandler.h"
#include "player.h"

class GameClient;

/*
 * Manage connections to game server.
 */
class GameHandler: public ConnectionHandler
{
    public:
        void process();

        /**
         * Send message to the given player.
         */
        void sendTo(Player *, MessageOut &msg);

    protected:
        NetComputer *computerConnected(ENetPeer *);
        void computerDisconnected(NetComputer *);

        /**
         * Process messages related to core game events.
         */
        void processMessage(NetComputer *computer, MessageIn &message);

    private:
        void removeOutdatedPending();
};

/**
 * Register future client attempt. Temporary until physical server split.
 */
void registerGameClient(std::string const &, PlayerPtr);

extern GameHandler *gameHandler;

#endif
