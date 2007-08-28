/*
 *  The Mana World
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

#ifndef _TMW_ACCOUNTCONNECTION_H_
#define _TMW_ACCOUNTCONNECTION_H_

#include "net/connection.hpp"

class Character;

/**
 * A connection to the account server.
 */
class AccountConnection : public Connection
{
    public:
        /**
         * Initializes a connection to the account server described in the
         * configuration file. Registers the maps known by MapManager.
         */
        bool start();

        /**
         * Sends data of a given character.
         */
        void sendCharacterData(Character *);

        /**
         * Prepares the account server for a reconnecting player
         */
        void playerReconnectAccount(int id, std::string const &magic_token);

        /**
         * Requests the value of a quest variable from the database.
         */
        void requestQuestVar(Character *, std::string const &);

        /**
         * Pushes a new quest value to the database.
         */
        void updateQuestVar(Character *, std::string const &name,
                            std::string const &value);

#if 0
        /**
         * Sends create guild message
         */
        void playerCreateGuild(int id, const std::string &guildName);
        
        /**
         * Sends invite message
         */
        void playerInviteToGuild(int id, short guildId, const std::string &name);
        
        /**
         * Sends accept message
         */
        void playerAcceptInvite(int id, const std::string &name);
        
        /**
         * Sends get guild members message.
         */
        void getGuildMembers(int id, short guildId);
        
        /**
         * Sends quit guild message.
         */
        void quitGuild(int id, short guildId);
#endif

    protected:
        /**
         * Processes server messages.
         */
        virtual void processMessage(MessageIn &);
};

extern AccountConnection *accountHandler;

#endif
