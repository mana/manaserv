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

#ifndef _TMWSERV_ACCOUNTCLIENT_H_
#define _TMWSERV_ACCOUNTCLIENT_H_

#include "netcomputer.h"
#include "account.h"

#include <enet/enet.h>

class AccountHandler;

/**
 * A connected computer that can have an account and character associated with
 * it.
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
        void
        setAccount(AccountPtr acc);

        /**
         * Unset the account associated with the connection
         */
        void
        unsetAccount();

        /**
         * Get account associated with the connection.
         */
        AccountPtr
        getAccount() const { return mAccountPtr; }

        /**
         * Set the selected character associated with connection.
         */
        void
        setCharacter(PlayerPtr ch);

        /**
         * Deselect the character associated with connection.
         */
        void
        unsetCharacter();

        /**
         * Get character associated with the connection
         */
        PlayerPtr
        getCharacter() const { return mCharacterPtr; }

    private:
        /** Account associated with connection */
        AccountPtr mAccountPtr;

        /** Selected character */
        PlayerPtr mCharacterPtr;
};

#endif
