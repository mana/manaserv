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
#include "state.h"
#include "configuration.h"
#include <iostream>
#include <cctype>

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

#if defined (SQLITE_SUPPORT)
    // Reopen the db in this thread for sqlite, to avoid
    // Library Call out of sequence problem due to thread safe.
    store.setUser(config.getValue("dbuser", ""));
    store.setPassword(config.getValue("dbpass", ""));
    store.close();
    store.open();
#endif

    MessageOut result;

    switch (message.getId())
    {
        case CMSG_LOGIN:
            {
                std::string username = message.readString();
                std::string password = message.readString();
                std::cout << username << " is trying to login." << std::endl;

                if (computer.getAccount() != NULL) {
                    result.writeShort(SMSG_LOGIN_ERROR);
                    result.writeShort(LOGIN_UNKNOWN);
                    break;
                }

                // see if the account exists
                Account *acc = store.getAccount(username);

                if (!acc) {
                    // account doesn't exist -- send error to client
                    std::cout << username << ": Account does not exist." << std::endl;

                    result.writeShort(SMSG_LOGIN_ERROR);
                    result.writeByte(LOGIN_INVALID_USERNAME);
                } else if (acc->getPassword() != password) {
                    // bad password -- send error to client
                    std::cout << "Bad password for " << username << std::endl;

                    result.writeShort(SMSG_LOGIN_ERROR);
                    result.writeByte(LOGIN_INVALID_PASSWORD);
                } else {
                    std::cout << "Login OK by " << username << std::endl;

                    // Associate account with connection
                    computer.setAccount(acc);

                    result.writeShort(SMSG_LOGIN_CONFIRM);

                    // Return information about available characters
                    tmwserv::Beings &chars = computer.getAccount()->getCharacters();
                    result.writeByte(chars.size());

                    for (unsigned int i = 0; i < chars.size(); i++) {
                        result.writeString(chars[i]->getName());
                        result.writeByte(chars[i]->getLevel());
                        result.writeByte(chars[i]->getMoney());
                        //result.writeString(chars[i]->getRawStatistics(),
                        //                   sizeof(tmwserv::RawStatistics));
                    }
                }
            }
            break;

        case CMSG_REGISTER:
            {
                std::string username = message.readString();
                std::string password = message.readString();
                std::string email = message.readString();

                // checking conditions for having a good account.
                std::cout << username << " is trying to register." << std::endl;

                bool emailValid = true;
                // looking the email address already exists.
                std::list<std::string> emailList = store.getEmailList();
                std::string upcasedEmail, upcasedIt;
                for (std::list<std::string>::const_iterator it = emailList.begin(); it != emailList.end(); it++)
                {
                    // Upcasing both mails for a good comparison
                    upcasedEmail = email;
                    std::transform(upcasedEmail.begin(), upcasedEmail.end(), upcasedEmail.begin(), (int(*)(int))std::toupper);
                    upcasedIt = *it;
                    std::transform(upcasedIt.begin(), upcasedIt.end(), upcasedIt.begin(), (int(*)(int))std::toupper);
                    if ( upcasedEmail == upcasedIt )
                    {
                        result.writeShort(SMSG_REGISTER_RESPONSE);
                        result.writeByte(REGISTER_EXISTS_EMAIL);
                        std::cout << email << ": Email already exists" << std::endl;
                        emailValid = false;
                        break;
                    }
                }
                if (!emailValid) break;

                // Testing Email validity
                emailValid = false;
                if ((email.find_first_of('@') != std::string::npos)) // Searching for an @.
                {
                    int atpos = email.find_first_of('@');
                    if (email.find_first_of('.', atpos) != std::string::npos) // Searching for a '.' after the @.
                    {
                        if (email.find_first_of(' ') == std::string::npos) // Searching if there's no spaces.
                        {
                            emailValid = true;
                        }
                    }
                }

                // see if the account exists
                Account *accPtr = store.getAccount(username);
                if ( accPtr ) // Account already exists.
                {
                    result.writeShort(SMSG_REGISTER_RESPONSE);
                    result.writeByte(REGISTER_EXISTS_USERNAME);
                    std::cout << username << ": Username already exists." << std::endl;
                }
                else if ((username.length() < 4) || (username.length() > 16)) // Username length
                {
                    result.writeShort(SMSG_REGISTER_RESPONSE);
                    result.writeByte(REGISTER_INVALID_USERNAME);
                    std::cout << username << ": Username too short or too long." << std::endl;
                }
                else if (password.length() < 4)
                {
                    result.writeShort(SMSG_REGISTER_RESPONSE);
                    result.writeByte(REGISTER_INVALID_PASSWORD);
                    std::cout << email << ": Password too short." << std::endl;
                }
                else if (!emailValid)
                {
                    result.writeShort(SMSG_REGISTER_RESPONSE);
                    result.writeByte(REGISTER_INVALID_EMAIL);
                    std::cout << email << ": Email Invalid (misses @, or . after the @, or maybe there is a ' '.)." << std::endl;
                }
                else
                {
                    AccountPtr acc(new Account(username, password, email));
                    store.addAccount(acc);

                    result.writeShort(SMSG_REGISTER_RESPONSE);
                    result.writeByte(REGISTER_OK);

                    std::cout << username << ": Account registered." << std::endl;
                    store.flush(); // flush changes
                }
            }
            break;

        case CMSG_CHAR_CREATE:
            {
                if (computer.getAccount() == NULL) {
                    result.writeShort(SMSG_CHAR_CREATE_RESPONSE);
                    result.writeByte(CREATE_NOLOGIN);
                    std::cout << "Not logged in. Can't create a Character." << std::endl;
                    break;
                }

                std::string name = message.readString();
                //char hairStyle = message.readByte();
                //char hairColor = message.readByte();
                Genders sex = (Genders)message.readByte();

                // TODO: Finish this message type (should a player customize stats
                // slightly?)
                // A player shouldn't have more than 3 characters.

                tmwserv::RawStatistics stats = {10, 10, 10, 10, 10, 10};
                tmwserv::BeingPtr newCharacter(new tmwserv::Being(name, sex, 1, 0, stats));
                computer.getAccount()->addCharacter(newCharacter);

                result.writeShort(SMSG_CHAR_CREATE_RESPONSE);
                result.writeByte(CREATE_OK);

                store.flush(); // flush changes
            }
            break;

        case CMSG_CHAR_SELECT:
            {
                if (computer.getAccount() == NULL)
                {
                    result.writeShort(SMSG_CHAR_SELECT_RESPONSE);
                    result.writeByte(SELECT_NOLOGIN);
                    std::cout << "Not logged in. Can't select a Character." << std::endl;
                    break; // not logged in
                }

                unsigned char charNum = message.readByte();

                tmwserv::Beings &chars = computer.getAccount()->getCharacters();

                result.writeShort(SMSG_CHAR_SELECT_RESPONSE);
                if (charNum >= chars.size()) {
                    // invalid char selection
                    result.writeByte(SELECT_INVALID);
                    break;
                }

                // set character
                computer.setCharacter(chars[charNum].get());

                // place in world
                tmwserv::State &state = tmwserv::State::instance();
                state.addBeing(computer.getCharacter(), computer.getCharacter()->getMap());

                result.writeByte(SELECT_OK);
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
