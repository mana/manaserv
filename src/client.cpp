#include <SDL.h>
#include <SDL_net.h>
#include <stdlib.h>
#include <iostream>
#include "defines.h"
#include "messageout.h"

int main(int argc, char *argv[])
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
        printf("SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }

    // Set SDL to quit on exit
    atexit(SDL_Quit);

    // Initialize SDL_net
    if (SDLNet_Init() == -1)
    {
        printf("SDLNet_Init: %s\n", SDLNet_GetError());
        exit(2);
    }

    // Try to connect to server
    IPaddress ip;
    TCPsocket tcpsock;

    if (SDLNet_ResolveHost(&ip, "localhost", 9601) == -1)
    {
        printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
        exit(1);
    }

    tcpsock = SDLNet_TCP_Open(&ip);
    if (!tcpsock)
    {
        printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
        exit(2);
    }

    printf("Succesfully connected!\n");

    int answer = 1;
    char line[256] = "";
    
    while (answer != 0)
    {

        MessageOut msg;
        
        printf ("0) Quit\n");
        printf ("1) Register\n");
        printf ("2) Login\n");
        printf ("4) Create character\n");
        printf ("3) Chat\n\n");
        printf ("Insert your option: ");
        std::cin >> answer;

        switch (answer) {
            case 1:
                // Register
                msg.writeShort(CMSG_REGISTER);
                printf("Account name: ", line);
                std::cin >> line;
                msg.writeString(line);
                printf("Password: ", line);
                std::cin >> line;
                msg.writeString(line);
                printf("Email address: ", line);
                std::cin >> line;
                msg.writeString(line);
                break;
            case 2:
                // Login
                msg.writeShort(CMSG_LOGIN);
                printf("Account name: ", line);
                std::cin >> line;
                msg.writeString(line);
                printf("Password: ", line);
                std::cin >> line;
                msg.writeString(line);
                break;
            case 3:
                // Chat
                msg.writeShort(CMSG_SAY);
                printf("\nChat: ", line);
                std::cin >> line;
                msg.writeString(line);
                msg.writeShort(0);
                break;
            case 4:
            {
                // Create character
                msg.writeShort(CMSG_CHAR_CREATE);
                printf("\nName: ");
                std::cin >> line;
                msg.writeString(line);
                msg.writeByte(0);
            } break;

            default:
                continue;
        }
        printf("\n");
        
        // Message hex
        for (unsigned int i = 0; i < msg.getPacket()->length; i++) {
            printf("%x ", msg.getPacket()->data[i]);
        }
        printf("\n\n");
        
        SDLNet_TCP_Send(tcpsock, msg.getPacket()->data,
                        msg.getPacket()->length);
                        
        if (answer != 3) {
            char data[1024];
            int recvLength = SDLNet_TCP_Recv(tcpsock, data, 1024);
            printf("Received:\n");
            if (recvLength != -1)
            {
                for (unsigned int i = 0; i < recvLength; i++) {
                    printf("%x ", data[i]);
                }
            }
            else {
                printf("ERROR!");
            }
            printf("\n\n");
        }
    }
    
    SDLNet_TCP_Close(tcpsock);

    return 0;
}
