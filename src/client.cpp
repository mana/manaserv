/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */

#include <SDL.h>
#include <SDL_net.h>
#include <iostream>
#include "defines.h"
#include "messageout.h"
#include "messagein.h"

int main(int argc, char *argv[])
{

#if (defined __USE_UNIX98 || defined __FreeBSD__)
#include "../config.h"
#endif
#ifdef PACKAGE_VERSION
    std::cout << "The Mana World Test Client v" << PACKAGE_VERSION << std::endl;
#else
    std::cout << "The Mana World Test Client v" << DEFAULT_PACKAGE_VERSION << std::endl;
#endif

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
        std::cout << "SDL_Init: " << SDL_GetError() << std::endl;
        exit(1);
    }

    // Set SDL to quit on exit
    atexit(SDL_Quit);

    // Initialize SDL_net
    if (SDLNet_Init() == -1) {
        std::cout << "SDLNet_Init: " << SDLNet_GetError() << std::endl;
        exit(2);
    }

    // Try to connect to server
    IPaddress ip;
    TCPsocket tcpsock;

    if (SDLNet_ResolveHost(&ip, "localhost", 9601) == -1) {
        std::cout << "SDLNet_ResolveHost: " << SDLNet_GetError() << std::endl;
        exit(1);
    }

    tcpsock = SDLNet_TCP_Open(&ip);
    if (!tcpsock) {
        std::cout << "SDLNet_TCP_Open: " << SDLNet_GetError() << std::endl;
        exit(2);
    }

    std::cout << "Successfully connected!" << std::endl;

    int answer = 1;
    char line[256] = "";

    while (answer != 0)
    {
        bool responseRequired = true;
        MessageOut msg;

        std::cout << "0) Quit                9)  Character selection" << std::endl;
        std::cout << "1) Register            10) Delete Character" << std::endl;
        std::cout << "2) Unregister          11) List Characters" << std::endl;
        std::cout << "3) Login               12) Move Character" << std::endl;
        std::cout << "4) Logout              13) Chat" << std::endl;
        std::cout << "5) Change Password     14) Equip Item" << std::endl;
        std::cout << "6) Change Email        15) Ruby Expression" << std::endl;
        std::cout << "7) Get Email" << std::endl;
        std::cout << "8) Create character" << std::endl;
        std::cout << "Choose your option: ";
        std::cin >> answer;

        switch (answer) {
            case 1:
                // Register
                msg.writeShort(CMSG_REGISTER);
                // We send the client version
#ifdef PACKAGE_VERSION
                msg.writeString(PACKAGE_VERSION);
#else
                msg.writeString(DEFAULT_PACKAGE_VERSION);
#endif
                std::cout << "Account name: ";
                std::cin >> line;
                msg.writeString(line);
                std::cout <<"Password: ";
                std::cin >> line;
                msg.writeString(line);
                std::cout << "Email address: ";
                std::cin >> line;
                msg.writeString(line);
                break;

            case 2:
                // Unregister (deleting an account)
                msg.writeShort(CMSG_UNREGISTER);
                std::cout << "Account name: ";
                std::cin >> line;
                msg.writeString(line);
                std::cout << "Password: ";
                std::cin >> line;
                msg.writeString(line);
                break;

            case 3:
                // Login
                msg.writeShort(CMSG_LOGIN);
                // We send the client version
#ifdef PACKAGE_VERSION
                msg.writeString(PACKAGE_VERSION);
#else
                msg.writeString(DEFAULT_PACKAGE_VERSION);
#endif
                std::cout << "Account name: ";
                std::cin >> line;
                msg.writeString(line);
                std::cout << "Password: ";
                std::cin >> line;
                msg.writeString(line);
                break;

            case 4:
                // Logout
                msg.writeShort(CMSG_LOGOUT);
                std::cout << "Logout" << std::endl;
                break;

            case 5:
                // Change Password
                msg.writeShort(CMSG_PASSWORD_CHANGE);
                std::cout << "Old Password: ";
                std::cin >> line;
                msg.writeString(line);
                std::cout << "New Password: ";
                std::cin >> line;
                msg.writeString(line);
                std::cout << "Retype new Password: ";
                std::cin >> line;
                msg.writeString(line);
                break;

            case 6:
                // Change Email
                msg.writeShort(CMSG_EMAIL_CHANGE);
                std::cout << "New Email: ";
                std::cin >> line;
                msg.writeString(line);
                break;

            case 7:
                // Get current Account's Email value
                msg.writeShort(CMSG_EMAIL_GET);
                break;

            case 8:
            {
                // Create character
                msg.writeShort(CMSG_CHAR_CREATE);
                std::cout << "Name: ";
                std::cin >> line;
                msg.writeString(line);

                std::cout << "Hair Style ID (0 - " << MAX_HAIRSTYLE_VALUE << "): ";
                std::cin >> line;
                msg.writeByte(atoi(line));

                std::cout << "Hair Color ID (0 - " << MAX_HAIRCOLOR_VALUE << "): ";
                std::cin >> line;
                msg.writeByte(atoi(line));

                std::cout << "Gender ID (0 - " << MAX_GENDER_VALUE << "): ";
                std::cin >> line;
                msg.writeByte(atoi(line));

                std::cout << "Strength: ";
                std::cin >> line;
                msg.writeShort(atoi(line));

                std::cout << "Agility: ";
                std::cin >> line;
                msg.writeShort(atoi(line));

                std::cout << "Vitality: ";
                std::cin >> line;
                msg.writeShort(atoi(line));

                std::cout << "Intelligence: ";
                std::cin >> line;
                msg.writeShort(atoi(line));

                std::cout << "Dexterity: ";
                std::cin >> line;
                msg.writeShort(atoi(line));

                std::cout << "Luck: ";
                std::cin >> line;
                msg.writeShort(atoi(line));
            } break;

            case 9:
            {
                // Select character
                msg.writeShort(CMSG_CHAR_SELECT);
                std::cout << "Character ID: ";
                std::cin >> line;
                msg.writeByte(atoi(line));
            } break;

            case 10:
            {
                // Delete character
                msg.writeShort(CMSG_CHAR_DELETE);
                std::cout << "Character ID: ";
                std::cin >> line;
                msg.writeByte(atoi(line));
            } break;

            case 11:
            {
                // List characters
                msg.writeShort(CMSG_CHAR_LIST);
            } break;

            case 12:
            {
                // Move character
                long x, y;
                std::cout << "X: ";
                std::cin >> x;
                std::cout << "Y: ";
                std::cin >> y;

                msg.writeShort(CMSG_WALK);
                msg.writeLong(x);
                msg.writeLong(y);

                responseRequired = false;
            } break;

            case 13:
                // Chat
                msg.writeShort(CMSG_SAY);
                std::cout << "Chat: ";
                std::cin >> line;
                msg.writeString(line);
                msg.writeShort(0);
                responseRequired = false;
                break;

            case 14:
            {
                // Equip
                unsigned int itemId;
                unsigned int slot;
                std::cout << "Item ID: ";
                std::cin >> itemId;
                std::cout << "Slot: ";
                std::cin >> slot;
                msg.writeShort(CMSG_EQUIP);
                msg.writeLong(itemId);
                msg.writeByte(slot);
            } break;

            case 15:
            {
                std::cout << "Expr: ";
                std::cin >> line;
                msg.writeShort(0x800);
                msg.writeString(line);

                responseRequired = false;
            } break;

            default:
                continue;
        }
        std::cout << "Sent: " << std::endl;

        // Message hex
        for (unsigned int i = 0; i < msg.getPacket()->length; i++) {
            std::cout << int(msg.getPacket()->data[i]) << " ";
        }
        std::cout << std::endl;

        SDLNet_TCP_Send(tcpsock, msg.getPacket()->data,
                        msg.getPacket()->length);

        // Raw Datas
        char data[1024];
        int recvLength = SDLNet_TCP_Recv(tcpsock, data, 1024);

        if (responseRequired) {
            std::cout << "Received: ";
            if (recvLength != -1) {
                for (unsigned int i = 0; i < recvLength; i++) {
                    std::cout << int(data[i]) << " ";
                }
            } else {
                std::cout << "ERROR!" << std::endl;
            }
            std::cout << std::endl;
        }

        // Response handling
        // Transforming it into a MessageIn object
        if (recvLength >= 2)
        {
            Packet *packet = new Packet(data, recvLength);
            MessageIn msg(packet); // (MessageIn frees packet)

            switch (msg.getId()) {
                case SMSG_REGISTER_RESPONSE:
                    // Register
                    switch (msg.readByte())
                    {
                        case REGISTER_OK:
                            std::cout << "Account registered." << std::endl;
                        break;
                        case REGISTER_INVALID_USERNAME:
                            std::cout << "Account registering: Invalid username." << std::endl;
                        break;
                        case REGISTER_INVALID_PASSWORD:
                            std::cout << "Account registering: Invalid password." << std::endl;
                        break;
                        case REGISTER_INVALID_EMAIL:
                            std::cout << "Account registering: Invalid Email." << std::endl;
                        break;
                        case REGISTER_INVALID_VERSION:
                            std::cout << "Account registering: Invalid version." << std::endl;
                        break;
                        case REGISTER_EXISTS_USERNAME:
                            std::cout << "Account registering: Username already exists." << std::endl;
                        break;
                        case REGISTER_EXISTS_EMAIL:
                            std::cout << "Account registering: Email already exists." << std::endl;
                        break;
                        default:
                        case REGISTER_UNKNOWN:
                            std::cout << "Account registering: Unknown error." << std::endl;
                        break;
                    }
                    break;

                case SMSG_UNREGISTER_RESPONSE:
                    // Register
                    switch (msg.readByte())
                    {
                        case UNREGISTER_OK:
                            std::cout << "Account unregistered." << std::endl;
                        break;
                        case UNREGISTER_INVALID_PASSWORD:
                            std::cout << "Account unregistering: Invalid password." << std::endl;
                        break;
                        case UNREGISTER_INVALID_USERNAME:
                            std::cout << "Account unregistering: Invalid username." << std::endl;
                        break;
                        case UNREGISTER_INVALID_UNSUFFICIENT_RIGHTS:
                            std::cout << "Account unregistering: unsufficient rights." << std::endl;
                        break;
                        default:
                        case UNREGISTER_UNKNOWN:
                            std::cout << "Account unregistering: Unknown error." << std::endl;
                        break;
                    }
                    break;

                case SMSG_LOGIN_RESPONSE:
                    // Register
                    switch (msg.readByte())
                    {
                        case LOGIN_OK:
                            char charNumber;
                            charNumber = msg.readByte();
                            std::cout << "Account has " << int(charNumber) << " characters." << std::endl;
                            for (unsigned int i = 0; i < charNumber; i++)
                            {
                                if (i >0) std::cout << ", ";
                                std::cout << msg.readString();
                            }
                            std::cout << "." << std::endl;
                        break;
                        case LOGIN_INVALID_USERNAME:
                            std::cout << "Login: Invalid Username." << std::endl;
                        break;
                        case LOGIN_INVALID_PASSWORD:
                            std::cout << "Login: Invalid Password." << std::endl;
                        break;
                        case LOGIN_INVALID_VERSION:
                            std::cout << "Login: Invalid Version." << std::endl;
                        break;
                        case LOGIN_ALREADY_LOGGED:
                            std::cout << "Login: Already logged with another account." << std::endl;
                        break;
                        case LOGIN_SERVER_FULL:
                            std::cout << "Login: Server has reached maximum of clients." << std::endl;
                        break;
                        case LOGIN_ACCOUNT_BANNED:
                            std::cout << "Login: Your account has been banned." << std::endl;
                        break;
                        case LOGIN_ACCOUNT_REVIEW:
                            std::cout << "TODO:Login: Your account is being reviewed." << std::endl;
                        break;
                        default:
                        case LOGIN_UNKNOWN:
                            std::cout << "Login: Unknown error." << std::endl;
                        break;
                    }
                    break;

                case SMSG_LOGOUT_RESPONSE:
                    {
                        switch (msg.readByte())
                        {
                            case LOGOUT_OK:
                                std::cout << "Logout..." << std::endl;
                            break;
                            default:
                            case LOGOUT_UNSUCCESSFULL:
                                std::cout << "Logout: unsuccessfull." << std::endl;
                            break;
                        }
                    }
                    break;

                case SMSG_PASSWORD_CHANGE_RESPONSE:
                    {
                        switch (msg.readByte())
                        {
                            case PASSCHG_OK:
                                std::cout << "Password correctly changed." << std::endl;
                            break;
                            case PASSCHG_NOLOGIN:
                                std::cout << "Password change: Not logged in." << std::endl;
                            break;
                            case PASSCHG_MISMATCH:
                                std::cout << "Password change: Passwords mismatch." << std::endl;
                            break;
                            case PASSCHG_INVALID:
                                std::cout << "Password change: New password is invalid." << std::endl;
                            break;
                            default:
                            case PASSCHG_UNKNOWN:
                                std::cout << "Password change: Unknown error." << std::endl;
                            break;
                        }
                    }
                    break;

                case SMSG_EMAIL_CHANGE_RESPONSE:
                    {
                        switch (msg.readByte())
                        {
                            case EMAILCHG_OK:
                                std::cout << "Email correctly changed." << std::endl;
                            break;
                            case EMAILCHG_NOLOGIN:
                                std::cout << "Email change: Not logged in." << std::endl;
                            break;
                            case EMAILCHG_EXISTS_EMAIL:
                                std::cout << "Email change: Email already exists." << std::endl;
                            break;
                            case EMAILCHG_INVALID:
                                std::cout << "Email change: New Email is invalid." << std::endl;
                            break;
                            default:
                            case EMAILCHG_UNKNOWN:
                                std::cout << "Email change: Unknown error." << std::endl;
                            break;
                        }
                    }
                    break;

                case SMSG_EMAIL_GET_RESPONSE:
                    {
                        switch (msg.readByte())
                        {
                            case EMAILGET_OK:
                                std::cout << "Current Email: " << msg.readString() << std::endl;
                            break;
                            case EMAILGET_NOLOGIN:
                                std::cout << "Get Email: Not logged in." << std::endl;
                            break;
                            default:
                            case EMAILGET_UNKNOWN:
                                std::cout << "Get Email: Unknown error." << std::endl;
                            break;
                        }
                    }
                    break;

                case SMSG_CHAR_CREATE_RESPONSE:
                    {
                        switch (msg.readByte())
                        {
                            case CREATE_OK:
                                std::cout << "Character Created successfully." << std::endl;
                            break;
                            case CREATE_EXISTS_NAME:
                                std::cout << "Character Creation: Character's name already exists."
                                << std::endl;
                            break;
                            case CREATE_NOLOGIN:
                                std::cout << "Character Creation: Not logged in." << std::endl;
                            break;
                            case CREATE_TOO_MUCH_CHARACTERS:
                                std::cout << "Character Creation: Too much characters." << std::endl;
                            break;
                            case CREATE_INVALID_HAIRSTYLE:
                                std::cout << "Character Creation: Invalid Hair Style Value." << std::endl;
                            break;
                            case CREATE_INVALID_HAIRCOLOR:
                                std::cout << "Character Creation: Invalid Hair Color Value." << std::endl;
                            break;
                            case CREATE_INVALID_GENDER:
                                std::cout << "Character Creation: Invalid Gender Value." << std::endl;
                            break;
                            case CREATE_INVALID_NAME:
                                std::cout << "Character Creation: Invalid Name." << std::endl;
                            break;
                            case CREATE_RAW_STATS_EQUAL_TO_ZERO:
                                std::cout << "Character Creation: a Statistic is equal to zero." << std::endl;
                            break;
                            case CREATE_RAW_STATS_INVALID_DIFF:
                                std::cout << "Character Creation: Statistics disproportionned." << std::endl;
                            break;
                            case CREATE_RAW_STATS_TOO_HIGH:
                                std::cout << "Character Creation: Statistics too high for level 1." << std::endl;
                            break;
                            case CREATE_RAW_STATS_TOO_LOW:
                                std::cout << "Character Creation: Statistics too low for level 1." << std::endl;
                            break;
                            default:
                            case CREATE_UNKNOWN:
                                std::cout << "Character Creation: Unknown error." << std::endl;
                            break;
                        }
                    }
                    break;

                case SMSG_CHAR_DELETE_RESPONSE:
                    {
                        switch (msg.readByte())
                        {
                            case DELETE_OK:
                                std::cout << "Character deleted." << std::endl;
                            break;
                            case DELETE_INVALID_NAME:
                                std::cout << "Character Deletion: Character's name doesn't exist."
                                << std::endl;
                            break;
                            case DELETE_NOLOGIN:
                                std::cout << "Character Deletion: Not logged in." << std::endl;
                            break;
                            case DELETE_NO_MORE_CHARACTERS:
                                std::cout << "Character Deletion: No more characters." << std::endl;
                            break;
                            default:
                            case DELETE_UNKNOWN:
                                std::cout << "Character Deletion: Unknown error." << std::endl;
                            break;
                        }
                    }
                    break;

                case SMSG_CHAR_SELECT_RESPONSE:
                    {
                        switch (msg.readByte())
                        {
                            case SELECT_OK:
                                std::cout << "Character selected." << std::endl;
                            break;
                            case SELECT_INVALID:
                                std::cout << "Character Selection: invalid ID."
                                << std::endl;
                            break;
                            case SELECT_NOLOGIN:
                                std::cout << "Character Selection: Not logged in." << std::endl;
                            break;
                            case SELECT_NOT_YET_CHARACTERS:
                                std::cout << "Character Selection: No character to select." << std::endl;
                            break;
                            default:
                            case SELECT_UNKNOWN:
                                std::cout << "Character Selection: Unknown error." << std::endl;
                            break;
                        }
                    }
                    break;

                case SMSG_CHAR_LIST_RESPONSE:
                    {
                        switch (msg.readByte())
                        {
                            case CHAR_LIST_OK:
                                char charNumber;
                                charNumber = msg.readByte();
                                std::cout << "Character List:" << std::endl
                                << "---------------" << std::endl;
                                std::cout << int(charNumber) << " characters in the account."
                                << std::endl;
                                for (unsigned int i = 0; i < charNumber; i++)
                                {
                                    std::cout << msg.readString() << ":" << std::endl;
                                    std::cout << "Gender: " << int(msg.readByte()) << ", ";
                                    std::cout << "Hair Style: " << int(msg.readByte()) << ", ";
                                    std::cout << "Hair Color: " << int(msg.readByte()) << ", "
                                    << std::endl;
                                    std::cout << "Level: " << int(msg.readByte()) << ", ";
                                    std::cout << "Money: " << int(msg.readShort()) << ", "
                                    << std::endl;
                                    std::cout << "Strength: " << int(msg.readShort()) << ", ";
                                    std::cout << "Agility: " << int(msg.readShort()) << ", ";
                                    std::cout << "Vitality: " << int(msg.readShort()) << ", "
                                    << std::endl;
                                    std::cout << "Intelligence: " << int(msg.readShort()) << ", ";
                                    std::cout << "Dexterity: " << int(msg.readShort()) << ", ";
                                    std::cout << "Luck: " << int(msg.readShort()) << ". "
                                    << std::endl << std::endl;
                                }
                            break;
                            case CHAR_LIST_NOLOGIN:
                                std::cout << "Character List: Not logged in."
                                << std::endl;
                            break;
                            default:
                            case CHAR_LIST_UNKNOWN:
                                std::cout << "Character List: Unknown error." << std::endl;
                            break;
                        }
                    }
                    break;


                default:
                    continue;
            } // End switch MessageId

        } // end if recLength > 2 (MessageLength > 2)


    } // End running loop

    SDLNet_TCP_Close(tcpsock);

    return 0;
}
