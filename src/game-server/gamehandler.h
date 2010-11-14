/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
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

#ifndef SERVER_GAMEHANDLER_H
#define SERVER_GAMEHANDLER_H

#include "game-server/character.h"
#include "net/connectionhandler.h"
#include "net/netcomputer.h"
#include "utils/tokencollector.h"

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
        void completeServerChange(int id, const std::string &token,
                                  const std::string &address, int port);

        /**
         * Updates the party id of the character
         */
        void updateCharacter(int charid, int partyid);

        /**
         * Registers a character that should soon be claimed by a client.
         * @param token token used by the client when connecting.
         */
        void addPendingCharacter(const std::string &token, Character *);

        /**
         * Combines a client with its character.
         * (Needed for TokenCollector)
         */
        void tokenMatched(GameClient *computer, Character *character);

        /**
         * Deletes a pending client's data.
         * (Needed for TokenCollector)
         */
        void deletePendingClient(GameClient *computer);

        /**
         * Deletes a pending connection's data.
         * (Needed for TokenCollector)
         */
        void deletePendingConnect(Character *character);

        /**
         * Gets the client associated to a character name. This method is slow,
         * so it should never be called for regular operations.
         */
        GameClient *getClientByNameSlow(const std::string &) const;

    protected:
        NetComputer *computerConnected(ENetPeer *);
        void computerDisconnected(NetComputer *);

        /**
         * Send error message back to player
         */
        void sendError(NetComputer *computer, int id, std::string errorMsg);

        /**
         * Processes messages related to core game events.
         */
        void processMessage(NetComputer *computer, MessageIn &message);

        /**
         * Set the position a player wants to move to
         */
        void handleWalk(GameClient *client, MessageIn &message);

        /**
         * Send a letter
         */
        void handleSendPost(GameClient *client, MessageIn &message);

        /**
         * Retrieve a letter
         */
        void handleGetPost(GameClient *client, MessageIn &message);

    private:

        /**
         * Container for pending clients and pending connections.
         */
        TokenCollector<GameHandler, GameClient *, Character *> mTokenCollector;

};

extern GameHandler *gameHandler;

#endif
