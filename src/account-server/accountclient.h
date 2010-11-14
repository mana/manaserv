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

#ifndef ACCOUNTCLIENT_H
#define ACCOUNTCLIENT_H

#include <enet/enet.h>

#include "account-server/account.h"
#include "net/netcomputer.h"

class AccountHandler;

enum AccountClientStatus
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

        AccountClientStatus status;

    private:
        /** Account associated with connection */
        Account *mAccount;
};

#endif
