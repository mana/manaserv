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

#include "netsession.h"
#include <SDL.h>
#include <SDL_net.h>

#define TMW_WORLD_TICK  SDL_USEREVENT
#define SERVER_PORT     9601

SDL_TimerID worldTimerID;          /**< Timer ID of world timer */
bool running = true;               /**< Determines if server keeps running */

/**
 * SDL timer callback, sends a <code>TMW_WORLD_TICK</code> event.
 */
Uint32 worldTick(Uint32 interval, void *param)
{
    // Push the custom world tick event
    SDL_Event event;
    event.type = TMW_WORLD_TICK;
    SDL_PushEvent(&event);
    return interval;
}

/**
 * Initializes the server.
 */
void initialize()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_TIMER) == -1) {
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

    // Initialize world timer at 10 times per second
    worldTimerID = SDL_AddTimer(100, worldTick, NULL);
}

/**
 * Deinitializes the server.
 */
void deinitialize()
{
    // Stop world timer
    SDL_RemoveTimer(worldTimerID);

    // Quit SDL_net
    SDLNet_Quit();
}

/**
 * Main function, initializes and runs server.
 */
int main(int argc, char *argv[])
{
    initialize();

    // Ready for server work...
    ConnectionHandler *connectionHandler = new ConnectionHandler();
    NetSession *session = new NetSession();

    session->startListen(connectionHandler, SERVER_PORT);

    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == TMW_WORLD_TICK)
            {
                // - Handle all messages that are in the message queue
                // - Update all active objects/beings
            }
            else if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }
    }

    session->stopListen(SERVER_PORT);

    deinitialize();
}
