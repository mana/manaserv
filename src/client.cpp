#include <SDL.h>
#include <SDL_net.h>
#include <stdlib.h>
#include "defines.h"
#include "messageout.h"

int main(int argc, char *argv[])
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
        printf("SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }

    // Set SDL to quit on exit
    atexit(SDL_Quit);

    // Initialize SDL_net
    if (SDLNet_Init() == -1) {
        printf("SDLNet_Init: %s\n", SDLNet_GetError());
        exit(2);
    }

    // Try to connect to server
    IPaddress ip;
    TCPsocket tcpsock;

    if (SDLNet_ResolveHost(&ip, "localhost", 9601) == -1) {
        printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
        exit(1);
    }

    tcpsock = SDLNet_TCP_Open(&ip);
    if (!tcpsock) {
        printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
        exit(2);
    }

    printf("Succesfully connected!\n");

    // Send a login message
    MessageOut msg;
    msg.writeByte(MSG_LOGIN);
    msg.writeString("nym");
    msg.writeString("password");

    SDLNet_TCP_Send(tcpsock, msg.getPacket()->data, msg.getPacket()->length);

    SDLNet_TCP_Close(tcpsock);

    return 0;
}
