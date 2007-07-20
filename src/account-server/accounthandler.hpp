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

#ifndef _TMWSERV_ACCOUNTHANDLER_H_
#define _TMWSERV_ACCOUNTHANDLER_H_

#include "net/connectionhandler.hpp"
#include "utils/tokencollector.hpp"

class AccountClient;
class CharacterData;

/**
 * Manages the data stored in user accounts and provides a reliable interface
 * for working with an account. The account handler class can be used as a link
 * to a working account handle, and can be assigned to a user persistently as
 * an interface between the computer and account. (Messages from the user can
 * be traced to this account through the NetComputer structure, then processed
 * here with the persistent stored data).
 */
class AccountHandler : public ConnectionHandler
{
    public:
        /**
         * Constructor
         */
        AccountHandler();

        /**
         * Start the handler
         */
        bool
        startListen(enet_uint16 port);

        /**
         * Combines a client with it's account.
         * (Needed for TokenCollector)
         */
        void
        tokenMatched(AccountClient *computer, int accountID);

        /**
         * Deletes a pending client's data.
         * (Needed for TokenCollector)
         */
        void
        deletePendingClient(AccountClient* computer);

        /**
         * Deletes a pending connection's data.
         * (Needed for TokenCollector)
         */
        void
        deletePendingConnect(int accountID);

        /**
         * TokenCollector, used to login a client without the client having to
         * send username and password a second time.
         */
        TokenCollector<AccountHandler, AccountClient*, int>
        mTokenCollector;

    protected:
        /**
         * Process account related messages.
         */
        void
        processMessage(NetComputer *computer, MessageIn &message);

        NetComputer*
        computerConnected(ENetPeer *peer);

        void
        computerDisconnected(NetComputer *comp);

        // --- message handling ---

        void
        handleLoginMessage(AccountClient &computer, MessageIn &msg);

        void
        handleLogoutMessage(AccountClient &computer);

        void
        handleReconnectMessage(AccountClient &computer, MessageIn &msg);

        void
        handleRegisterMessage(AccountClient &computer, MessageIn &msg);

        void
        handleUnregisterMessage(AccountClient &computer, MessageIn &msg);

        void
        handleEmailChangeMessage(AccountClient &computer, MessageIn &msg);

        void
        handleEmailGetMessage(AccountClient &computer);

        void
        handlePasswordChangeMessage(AccountClient &computer, MessageIn &msg);

        void
        handleCharacterCreateMessage(AccountClient &computer, MessageIn &msg);

        void
        handleCharacterSelectMessage(AccountClient &computer, MessageIn &msg);

        void
        handleCharacterDeleteMessage(AccountClient &computer, MessageIn &msg);

        /**
         * Send guild join for each guild the player belongs to.
         */
        void
        handleGuildJoining(AccountClient &computer, CharacterData *character);
};

extern AccountHandler * accountHandler;

#endif
