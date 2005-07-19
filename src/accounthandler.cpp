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

#include "accounthandler.h"
#include "debug.h"
#include "storage.h"
#include "account.h"
#include "messageout.h"
#include <iostream>

using tmwserv::Account;
using tmwserv::AccountPtr;
using tmwserv::Storage;

/**
 * Generic interface convention for getting a message and sending it to the
 * correct subroutines. Account handler takes care of determining the
 * current step in the account process, be it creation, setup, or login.
 */
void AccountHandler::receiveMessage(NetComputer &computer, MessageIn &message)
{
    Storage &store = Storage::instance("tmw");

    MessageOut result;

    switch (message.getId())
    {
        case CMSG_LOGIN:
            {
                std::string username = message.readString();
                std::string password = message.readString();

                // see if the account exists
                Account *acc = store.getAccount(username);
                if (!acc) {
                    // account doesn't exist -- send error to client
                    std::cout << "Account does not exist " << username << std::endl;

                    result.writeShort(SMSG_LOGIN_ERROR);
                    result.writeByte(LOGIN_INVALID_USERNAME);
                } else if (acc->getPassword() != password) {
                    // bad password -- send error to client
                    std::cout << "Bad password for " << username << std::endl;

                    result.writeShort(SMSG_LOGIN_ERROR);
                    result.writeByte(LOGIN_INVALID_PASSWORD);
                } else {
                    // Login OK! (send an OK message or something)
                    std::cout << "Login OK by " << username << std::endl;

                    result.writeShort(SMSG_LOGIN_CONFIRM);
                    // TODO: Return information about available characters
                }
            }
            break;

        case CMSG_REGISTER:
            {
                std::string username = message.readString();
                std::string password = message.readString();
                std::string email = message.readString();

                AccountPtr acc(new Account(username, password, email));
                store.addAccount(acc);

                result.writeShort(SMSG_REGISTER_RESPONSE);
                result.writeByte(REGISTER_OK);

                std::cout << "Account registered" << std::endl;
                store.flush(); // flush changes
            }
            break;

        case CMSG_CHAR_CREATE:
            {
                std::string name = message.readString();
                //char hairStyle = message.readByte();
                //char hairColor = message.readByte();
                //char sex = message.readByte();

                // TODO: Finish this message type (should a player customize stats
                // slightly?)

                result.writeShort(SMSG_CHAR_CREATE_RESPONSE);
                result.writeShort(CREATE_OK);
            }
            break;

        default:
            std::cout << "Invalid message type" << std::endl;
            result.writeShort(SMSG_LOGIN_ERROR);
            result.writeByte(LOGIN_UNKNOWN);
            break;
    }

    // return result
    computer.send(result.getPacket());
}

/* ----Login Message----
 * Accepts a login message and interprets it, assigning the proper
 * login
 * Preconditions: The requested handle is not logged in already. 
 *                The requested handle exists. 
 *                The requested handle is not banned or restricted. 
 *                The character profile is valid
 * Postconditions: The player recieves access through a character in
 *                 the world.
 * Return Value: SUCCESS if the player was successfully assigned the
 *               requested char, ERROR on early termination of the
 *               routine.
 */
int AccountHandler::loginMessage(NetComputer &computer, MessageIn &message)
{
    // Get the handle (account) the player is requesting
    // RETURN TMW_ACCOUNTERROR_NOEXIST if: requested does not handle exist
    // RETURN TMW_ACCOUNTERROR_BANNED if: the handle status is
    // HANDLE_STATUS_BANNED
    // RETURN TMW_ACCOUNTERROR_ALREADYASSIGNED if: the handle is already
    // assigned

    // Get the character within that handle that the player is requesting
    // RETURN TMW_ACCOUNTERROR_CHARNOTFOUND if: character not found

    // Assign the player to that character
    // RETURN TMW_ACCOUNTERROR_ASSIGNFAILED if: assignment not successful

    // return TMW_SUCCESS -- successful exit
    return TMW_SUCCESS;
}

/* ----Account Assignment----
 * Assigns the computer to this account, and allows it to make account
 * changes using this structure.
 * Preconditions: This structure already contains a valid accountHandle
 * Postconditions: The player is connected to the account through this handle
 * Return Value: SUCCESS if the player was successfully assigned the
 *               requested handle, ERROR on early termination of the
 *               routine.
 */
int
AccountHandler::assignAccount(NetComputer &computer, tmwserv::Account *account)
{
    // RETURN TMW_ACCOUNTERROR_ASSIGNFAILED if: the account was accessed before
    //                                          being initalized.

    // Assign the handle


    return TMW_SUCCESS;
}
