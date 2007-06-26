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

#include "game-server/character.hpp"
#include "net/connectionhandler.hpp"
#include "net/netcomputer.hpp"
#include "utils/tokencollector.hpp"

enum
{
    CLIENT_LOGIN = 0,
    CLIENT_CONNECTED,
    CLIENT_CHANGE_SERVER,
    CLIENT_QUEUED
};

struct GameClient: NetComputer
{
    GameClient(ENetPeer *peer)
      : NetComputer(peer), character(NULL), status(CLIENT_LOGIN) {}
    Character *character;
    int status;
};

/**
 * Manages connections to game client.
 */
class GameHandler: public ConnectionHandler
{
    public:
        /**
         * Constructor
         */
        GameHandler();
        /**
         * Processes messages and cleans outdated characters.
         */
        void process();

        /**
         * Starts the handler
         */
        bool startListen(enet_uint16 port);

        /**
         * Sends message to the given character.
         */
        void sendTo(Character *, MessageOut &msg);

        /**
         * Kills connection with given character.
         */
        void kill(Character *);

        /**
         * Prepares a server change for given character.
         */
        void prepareServerChange(Character *);

        /**
         * Completes a server change for given character ID.
         */
        void completeServerChange(int id, std::string const &token,
                                  std::string const &address, int port);
        
        /**
         * Map of character's and their id used for getting which character to
         * forward account server messages back to.
         */
        std::map<int, Character*> messageMap;

        /**
         * Combines a client with it's character.
         * (Needed for TokenCollector)
         */
        void
        tokenMatched(GameClient* computer, Character* character);

        /**
         * Deletes a pending client's data.
         * (Needed for TokenCollector)
         */
        void
        deletePendingClient(GameClient* computer);

        /**
         * Deletes a pending connection's data.
         * (Needed for TokenCollector)
         */
        void
        deletePendingConnect(Character* character);

        /**
         * TokenCollector, used to match a gameclient with the data received
         * from the accountserver.
         */
        TokenCollector<GameHandler, GameClient*, Character*>
        mTokenCollector;

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
