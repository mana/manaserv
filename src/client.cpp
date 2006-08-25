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

#include <iostream>
#include <enet/enet.h>

#include "defines.h"
#include "messageout.h"
#include "messagein.h"

#if defined WIN32
#include "../testclient_private.h"
#define PACKAGE_VERSION PRODUCT_VERSION
#endif

void parsePacket(char *data, int recvLength);

ENetHost *client;
ENetAddress addressAccount, addressGame, addressChat;
ENetPeer *peerAccount, *peerGame, *peerChat;
std::string token;
bool connected = false;

int main(int argc, char *argv[])
{

#if (defined __USE_UNIX98 || defined __FreeBSD__)
#include "../config.h"
#endif
    std::cout << "The Mana World Test Client v" << PACKAGE_VERSION << std::endl;

    if (enet_initialize () != 0)
    {
        printf("An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }

    atexit(enet_deinitialize);

    client = enet_host_create(NULL /* create a client host */,
                              3 /* allows 3 outgoing connection */,
                              57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
                              14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);

    if (client == NULL)
    {
        printf("An error occurred while trying to create an ENet client host.\n");
        exit (EXIT_FAILURE);
    }

    /* Connect to localhost:9601. */
    enet_address_set_host(&addressAccount, "localhost");
    addressAccount.port = 9601;

    /* Initiate the connection, allocating one channel. */
    peerAccount = enet_host_connect(client, &addressAccount, 1);

    if (peerAccount == NULL)
    {
       printf("No available peers for initiating an ENet connection.\n");
       exit (EXIT_FAILURE);
    }

    ENetEvent event;
    bool exit = true;
    char line[256] = "";

    printf("Starting client...\n");

    /* Wait up to 1000 milliseconds for an event. */
    do {
        if (connected) {
            int answer = -1;
            std::cout << std::endl;
            std::cout << "0) Quit                9)  Select Character" << std::endl;
            std::cout << "1) Register            10) Delete Character" << std::endl;
            std::cout << "2) Unregister" << std::endl;
            std::cout << "3) Login               12) Move Character" << std::endl;
            std::cout << "4) Logout              13) Say Around" << std::endl;
            std::cout << "5) Change Password     14) Equip Item" << std::endl;
            std::cout << "6) Change Email        15) Ruby Expression" << std::endl;
            std::cout << "7) Get Email           16) Enter Game Server" << std::endl;
            std::cout << "8) Create character    17) Enter Chat Server" << std::endl;
            std::cout << "Choose your option: ";
            std::cin >> answer;
            std::cin.getline(line, 256); // skip the remaining of the line

            MessageOut msg;
            int msgDestination = 0; // account server

            switch (answer) {
                case 0:
                    // Disconnection
                    if (connected) {
                        enet_peer_disconnect(&client->peers[0], 0);
                    }
                    exit = true;
                    break;

                case 1:
                    // Register
                    msg.writeShort(PAMSG_REGISTER);
                    // We send the client version
                    msg.writeLong(0);
                    std::cout << "Account name: ";
                    std::cin >> line;
                    msg.writeString(line);
                    std::cout << "Password: ";
                    std::cin >> line;
                    msg.writeString(line);
                    std::cout << "Email address: ";
                    std::cin >> line;
                    msg.writeString(line);
                    break;

                case 2:
                    // Unregister (deleting an account)
                    msg.writeShort(PAMSG_UNREGISTER);
                    std::cout << "Account name: ";
                    std::cin >> line;
                    msg.writeString(line);
                    std::cout << "Password: ";
                    std::cin >> line;
                    msg.writeString(line);
                    break;

                case 3:
                    // Login
                    msg.writeShort(PAMSG_LOGIN);
                    // We send the client version
                    msg.writeLong(0);
                    std::cout << "Account name: ";
                    std::cin >> line;
                    msg.writeString(line);
                    std::cout << "Password: ";
                    std::cin >> line;
                    msg.writeString(line);
                    break;

                case 4:
                    // Logout
                    msg.writeShort(PAMSG_LOGOUT);
                    std::cout << "Logout" << std::endl;
                    break;

                case 5:
                {
                    // Change Password
                    msg.writeShort(PAMSG_PASSWORD_CHANGE);
                    std::cout << "Old Password: ";
                    std::cin >> line;
                    msg.writeString(line);
                    std::cout << "New Password: ";
                    std::cin >> line;
                    msg.writeString(line);
                    std::string line2;
                    std::cout << "Retype new Password: ";
                    std::cin >> line2;
                    if (line != line2)
                    {
                        std::cout << "Error: Password mismatch." << std::endl;
                        goto process_enet;
                    }
                } break;

                case 6:
                    // Change Email
                    msg.writeShort(PAMSG_EMAIL_CHANGE);
                    std::cout << "New Email: ";
                    std::cin >> line;
                    msg.writeString(line);
                    break;

                case 7:
                    // Get current Account's Email value
                    msg.writeShort(PAMSG_EMAIL_GET);
                    break;

                case 8:
                {
                    // Create character
                    msg.writeShort(PAMSG_CHAR_CREATE);
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
                    msg.writeShort(PAMSG_CHAR_SELECT);
                    std::cout << "Character ID: ";
                    std::cin >> line;
                    msg.writeByte(atoi(line));
                } break;

                case 10:
                {
                    // Delete character
                    msg.writeShort(PAMSG_CHAR_DELETE);
                    std::cout << "Character ID: ";
                    std::cin >> line;
                    msg.writeByte(atoi(line));
                } break;

                case 12:
                {
                    // Move character
                    long x, y;
                    std::cout << "X: ";
                    std::cin >> x;
                    std::cout << "Y: ";
                    std::cin >> y;

                    msg.writeShort(PGMSG_WALK);
                    msg.writeShort(x);
                    msg.writeShort(y);

                    msgDestination = 1;
                } break;

                case 13:
                {
                    // Chat
                    msg.writeShort(PGMSG_SAY);
                    std::cout << "Say: ";
                    std::cin.getline(line, 256);
                    line[255] = '\0';
                    msg.writeString(line);

                    msgDestination = 1;
                }   break;

                case 14:
                {
                    // Equip
                    unsigned int itemId;
                    unsigned int slot;
                    std::cout << "Item ID: ";
                    std::cin >> itemId;
                    std::cout << "Slot: ";
                    std::cin >> slot;
                    msg.writeShort(PGMSG_EQUIP);
                    msg.writeLong(itemId);
                    msg.writeByte(slot);

                    msgDestination = 1;
                } break;

                case 15:
                {
                    std::cout << "Expr: ";
                    std::cin >> line;
                    msg.writeShort(0x800);
                    msg.writeString(line);
                } break;

                case 16:
                {
                    // enter game server
                    msg.writeShort(PGMSG_CONNECT);
                    msg.writeString(token, 32);
                    msgDestination = 1;
                } break;

                case 17:
                {
                    // enter chat server
                    msg.writeShort(PCMSG_CONNECT);
                    msg.writeString(token, 32);
                    msgDestination = 2;
                } break;

                default:
                    goto process_enet;
            } // end switch

            // Send prepared message
            if (!exit && connected) {
                ENetPacket *packet = enet_packet_create(
                        msg.getData(), msg.getLength(),
                        ENET_PACKET_FLAG_RELIABLE);
                ENetPeer *peer = peerAccount;
                if (msgDestination == 1) peer = peerGame;
                else if (msgDestination == 2) peer = peerChat;
                if (peer) {
                    // Send the packet to the peer over channel id 0.
                    enet_peer_send(peer, 0, packet);
                } else
                    std::cout << "Peer " << msgDestination << " is not connected. "
                                 "Cannot send packet." << std::endl;
            } // end if
        } // end if

        process_enet:
        while (enet_host_service(client, &event, 2000)) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("Connected to server\n");
                    connected = true;
                    exit = false;
                    break;

                case ENET_EVENT_TYPE_RECEIVE:
                    std::cout << "A packet of length "
                              << event.packet->dataLength
                              << " was received from "
                              << event.peer->address.host
                              << std::endl;

                    parsePacket((char *)event.packet->data,
                                event.packet->dataLength);

                    // Clean up the packet now that we're done using it.
                    enet_packet_destroy(event.packet);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    printf("Disconnected.\n");
                    connected = false;
                    exit = true;
                    break;

                default:
                    //printf("Unhandled enet event.\n");
                    break;
            } // end switch
        } // end while
    } while (!exit);

    if (connected) {
        // The disconnection attempt didn't succeed yet. Force disconnection.
        enet_peer_reset(&client->peers[0]);
    }

    enet_host_destroy(client);

    return 0;
}

void parsePacket(char *data, int recvLength) {
    // Response handling
    // Transforming it into a MessageIn object
    if (recvLength >= 2) {
        MessageIn msg(data, recvLength);

        switch (msg.getId()) {
            case APMSG_REGISTER_RESPONSE:
                // Register
                switch (msg.readByte()) {
                    case ERRMSG_OK:
                        std::cout << "Account registered." << std::endl;
                        break;
                    case ERRMSG_INVALID_ARGUMENT:
                        std::cout << "Account registering: Invalid username, password, or email." << std::endl;
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
                        std::cout << "Account registering: Unknown error." << std::endl;
                        break;
                }
                break;

            case APMSG_UNREGISTER_RESPONSE:
                // Register
                switch (msg.readByte()) {
                    case ERRMSG_OK:
                        std::cout << "Account unregistered." << std::endl;
                        break;
                    case ERRMSG_INVALID_ARGUMENT:
                        std::cout << "Account unregistering: Invalid username or password." << std::endl;
                        break;
                     default:
                        std::cout << "Account unregistering: Unknown error." << std::endl;
                        break;
                }
                break;

            case APMSG_LOGIN_RESPONSE:
                // Register
                switch (msg.readByte()) {
                    case ERRMSG_OK:
                        std::cout << "Login successful." << std::endl;
                        break;
                    case ERRMSG_INVALID_ARGUMENT:
                        std::cout << "Login: Invalid username or password." << std::endl;
                        break;
                    case LOGIN_INVALID_VERSION:
                        std::cout << "Login: Invalid Version." << std::endl;
                        break;
                    case ERRMSG_FAILURE:
                        std::cout << "Login: Already logged with another account." << std::endl;
                        break;
                    default:
                        std::cout << "Login: Unknown error." << std::endl;
                        break;
                }
                break;

            case APMSG_LOGOUT_RESPONSE:
            {
                switch (msg.readByte()) {
                    case ERRMSG_OK:
                        std::cout << "Logout..." << std::endl;
                        break;
                    case ERRMSG_NO_LOGIN:
                        std::cout << "Logout: Unsuccessful." << std::endl;
                        break;
                    default:
                        std::cout << "Logout: Unknown error." << std::endl;
                }
            } break;

            case APMSG_PASSWORD_CHANGE_RESPONSE:
            {
                switch (msg.readByte()) {
                    case ERRMSG_OK:
                        std::cout << "Password correctly changed." << std::endl;
                        break;
                    case ERRMSG_NO_LOGIN:
                        std::cout << "Password change: Not logged in." << std::endl;
                        break;
                    case ERRMSG_INVALID_ARGUMENT:
                        std::cout << "Password change: New password is invalid." << std::endl;
                        break;
                    case ERRMSG_FAILURE:
                        std::cout << "Password change: Old password is invalid." << std::endl;
                        break;
                    default:
                        std::cout << "Password change: Unknown error." << std::endl;
                        break;
                }
            } break;

            case APMSG_EMAIL_CHANGE_RESPONSE:
            {
                switch (msg.readByte()) {
                    case ERRMSG_OK:
                        std::cout << "Email correctly changed." << std::endl;
                        break;
                    case ERRMSG_NO_LOGIN:
                        std::cout << "Email change: Not logged in." << std::endl;
                        break;
                    case EMAILCHG_EXISTS_EMAIL:
                        std::cout << "Email change: Email already exists." << std::endl;
                        break;
                    case ERRMSG_INVALID_ARGUMENT:
                        std::cout << "Email change: New Email is invalid." << std::endl;
                        break;
                    default:
                        std::cout << "Email change: Unknown error." << std::endl;
                        break;
                }
            } break;

            case APMSG_EMAIL_GET_RESPONSE:
            {
                switch (msg.readByte()) {
                    case ERRMSG_OK:
                        std::cout << "Current Email: " << msg.readString() << std::endl;
                        break;
                    case ERRMSG_NO_LOGIN:
                        std::cout << "Get Email: Not logged in." << std::endl;
                        break;
                    default:
                        std::cout << "Get Email: Unknown error." << std::endl;
                        break;
                }
            } break;

            case APMSG_CHAR_CREATE_RESPONSE:
            {
                switch (msg.readByte()) {
                    case ERRMSG_OK:
                        std::cout << "Character Created successfully." << std::endl;
                        break;
                    case CREATE_EXISTS_NAME:
                        std::cout << "Character Creation: Character's name already exists."
                        << std::endl;
                        break;
                    case ERRMSG_NO_LOGIN:
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
                        std::cout << "Character Creation: Unknown error." << std::endl;
                        break;
                }
            } break;

            case APMSG_CHAR_DELETE_RESPONSE:
            {
                switch (msg.readByte()) {
                    case ERRMSG_OK:
                        std::cout << "Character deleted." << std::endl;
                        break;
                    case ERRMSG_INVALID_ARGUMENT:
                        std::cout << "Character Deletion: Character's ID doesn't exist."
                        << std::endl;
                        break;
                    case ERRMSG_NO_LOGIN:
                        std::cout << "Character Deletion: Not logged in." << std::endl;
                        break;
                    default:
                        std::cout << "Character Deletion: Unknown error." << std::endl;
                        break;
                }
            } break;

            case APMSG_CHAR_SELECT_RESPONSE:
            {
                switch (msg.readByte()) {
                    case ERRMSG_OK:
                    {
                        std::cout << "Character selected successfully." << std::endl;
                        std::cout << "Current Map: " << msg.readString() << std::endl;
                        std::string server = msg.readString();
                        enet_address_set_host(&addressGame, server.c_str());
                        addressGame.port = msg.readShort();
                        peerGame = enet_host_connect(client, &addressGame, 1);
                        std::cout << "Connecting to " << server << ':' << addressGame.port;
                        server = msg.readString();
                        enet_address_set_host(&addressChat, server.c_str());
                        addressChat.port = msg.readShort();
                        peerChat = enet_host_connect(client, &addressChat, 1);
                        token = msg.readString(32);
                        connected = false;
                        std::cout << " and to " << server << ':' << addressChat.port << std::endl;
                    } break;
                    case ERRMSG_INVALID_ARGUMENT:
                        std::cout << "Character Selection: invalid ID."
                        << std::endl;
                        break;
                    case ERRMSG_NO_LOGIN:
                        std::cout << "Character Selection: Not logged in." << std::endl;
                        break;
                    default:
                        std::cout << "Character Selection: Unknown error." << std::endl;
                        break;
                }
            } break;

            case APMSG_CHAR_INFO:
            {
                std::cout << "Information on character " << int(msg.readByte()) << std::endl;
                std::cout << "  Name: " << msg.readString() << std::endl;
                std::cout << "  Gender: " << int(msg.readByte()) << ", ";
                std::cout << "Hair Style: " << int(msg.readByte()) << ", ";
                std::cout << "Hair Color: " << int(msg.readByte()) << std::endl;
                std::cout << "  Level: " << int(msg.readByte()) << ", ";
                std::cout << "Money: " << int(msg.readShort()) << std::endl;
                std::cout << "  Strength: " << int(msg.readShort()) << ", ";
                std::cout << "Agility: " << int(msg.readShort()) << ", ";
                std::cout << "Vitality: " << int(msg.readShort()) << std::endl;
                std::cout << "  Intelligence: " << int(msg.readShort()) << ", ";
                std::cout << "Dexterity: " << int(msg.readShort()) << ", ";
                std::cout << "Luck: " << int(msg.readShort()) << std::endl;
                //std::cout << "  Current Map: " << msg.readString() << " (X:";
                //std::cout << int(msg.readShort()) << ", Y:";
                //std::cout << int(msg.readShort()) << ")" << std::endl;
            } break;

            case GPMSG_SAY:
            {
                std::string who = msg.readString();
                std::cout << who << " says around:" << std::endl
                << msg.readString() << std::endl;
            } break;

            case GPMSG_BEING_LEAVE:
            {
                switch (msg.readByte()) {
                case OBJECT_PLAYER:
                    std::cout << "Player " << msg.readLong() << " left map." << std::endl;
                    break;
                default:
                    std::cout << "Unknown being left map." << std::endl;
                }
            } break;

            case GPMSG_BEING_ENTER:
            {
                switch (msg.readByte()) {
                case OBJECT_PLAYER:
                    std::cout << "Player " << msg.readLong() << " entered map" << std::endl;
                    std::cout << "  name: " << msg.readString() << std::endl;
                    std::cout << "  hair style: " << (int)msg.readByte() << std::endl;
                    std::cout << "  hair color: " << (int)msg.readByte() << std::endl;
                    std::cout << "  gender: " << (int)msg.readByte() << std::endl;
                    break;
                default:
                    std::cout << "Unknown being entered map." << std::endl;
                }
            } break;

            case GPMSG_BEINGS_MOVE:
            {
                int nb = (recvLength - 2) / (1*4 + 4*2);
                std::cout << "Beings are moving:" << std::endl;
                for(; nb > 0; --nb) {
                    int id = msg.readLong();
                    int cx = msg.readShort(), cy = msg.readShort();
                    int dx = msg.readShort(), dy = msg.readShort();
                    std::cout << "  ID " << id << " at ("
                              << cx << ", " << cy << ") toward ("
                              << dx << ", " << dy << ")." << std::endl;
                }
            } break;

            case XXMSG_INVALID:
                std::cout << "The server does not understand our message." << std::endl;
                break;
            default:
                std::cout << "Unknown message received. Id: " << msg.getId() << "." << std::endl;
                break;
        } // end switch MessageId

    } // end if recLength > 2 (MessageLength > 2)

}

