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
 */

#ifndef _TMWSERV_ACCOUNTCLIENT_H_
#define _TMWSERV_ACCOUNTCLIENT_H_

#include <enet/enet.h>

#include "account-server/account.hpp"
#include "net/netcomputer.hpp"

class AccountHandler;

enum
{
    CLIENT_LOGIN = 0,
    CLIENT_CONNECTED,
    CLIENT_QUEUED
};

/**
 * A connected computer with an associated account.
 */
class AccountClient : public NetComputer
{
    public:
        /**
         * Constructor.
         */
        AccountClient(ENetPeer *peer);

        /**
         * Destructor.
         */
        ~AccountClient();

        /**
         * Set the account associated with the connection
         */
        void setAccount(Account *acc);

        /**
         * Unset the account associated with the connection
         */
        void unsetAccount();

        /**
         * Get account associated with the connection.
         */
        Account *getAccount() const
        { return mAccount; }

        /**
         * Update lastLoginAttempt
         */
        void updateLoginAttempt();

        /**
         * Returns the time of the last login attempt.
         */
        int getLastLoginAttempt() const
        { return lastLoginAttempt; }

        int status;

    private:
        /** Account associated with connection */
        Account *mAccount;
        time_t lastLoginAttempt;
};

#endif
