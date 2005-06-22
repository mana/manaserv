
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

#ifndef _TMW_SERVER_ACCOUNTHANDLER_
#define _TMW_SERVER_ACCOUNTHANDLER_

#include "messagehandler.h"
#include "netcomputer.h"
#include "messagein.h"
#include "account.h"

/**
 * Manages the data stored in user accounts and provides a reliable interface
 * for working with an account. The account handler class can be used as a link
 * to a working account handle, and can be assigned to a user persistently as 
 * an interface between the computer and account. (Messages from the user can 
 * be traced to this account through the NetComputer structure, then processed
 * here with the persistent stored data).
 */
class AccountHandler : public MessageHandler
{
    public:
        /**
         * Receives account related messages.
         */
        void receiveMessage(NetComputer &computer, MessageIn &message);

    private:
        /**
         * Handles the login message.
         */
        int loginMessage(NetComputer &computer, MessageIn &message);

        /**
         * Account assignment.
         */
        int assignAccount(NetComputer &computer, tmwserv::Account *account);
};

#endif
