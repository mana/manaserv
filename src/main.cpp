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

#define TMWSERV_VERSION "0.0.1"

#include <iostream>
#include "storage.h"
#include "netsession.h"
#include "connectionhandler.h"
#include "accounthandler.h"
#include <SDL.h>
#include <SDL_net.h>

#include "skill.h"

#include "log.h"
#define LOG_FILE "tmwserv.log"

// Scripting
#ifdef SCRIPT_SUPPORT

#include "script.h"
#define SCRIPT_SQUIRREL_SUPPORT

#ifdef SCRIPT_SQUIRREL_SUPPORT
#include "script-squirrel.h"
#endif
#ifdef SCRIPT_RUBY_SUPPORT
#include "script-ruby.h"
#endif
#ifdef SCRIPT_LUA_SUPPORT
#include "script-lua.h"
#endif

std::string scriptLanguage = "squirrel";
#endif
//

#define TMW_WORLD_TICK  SDL_USEREVENT
#define SERVER_PORT     9601

SDL_TimerID worldTimerID;          /**< Timer ID of world timer */
int worldTime = 0;                 /**< Current world time in 100ms ticks */
bool running = true;               /**< Determines if server keeps running */

Skill skillTree("base");           /**< Skill tree */

Storage store;

/**
 * SDL timer callback, sends a <code>TMW_WORLD_TICK</code> event.
 */
Uint32 worldTick(Uint32 interval, void *param)
{
    // Push the custom world tick event
    SDL_Event event;
    event.type = TMW_WORLD_TICK;
    if (SDL_PushEvent(&event)) {
        logger->log("Warning: couldn't push world tick into event queue!");
    }
    return interval;
}

/**
 * Initializes the server.
 */
void initialize()
{
    // Initialize the logger
    logger = new Logger(LOG_FILE);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
        logger->log("SDL_Init: %s", SDL_GetError());
        exit(1);
    }

    // Set SDL to quit on exit
    atexit(SDL_Quit);

    // Initialize SDL_net
    if (SDLNet_Init() == -1) {
        logger->log("SDLNet_Init: %s", SDLNet_GetError());
        exit(2);
    }

    // Initialize world timer at 10 times per second
    worldTimerID = SDL_AddTimer(100, worldTick, NULL);

    // Initialize scripting subsystem
#ifdef SCRIPT_SUPPORT
    logger->log("Script Language %s", scriptLanguage.c_str());

    if (scriptLanguage == "squirrel")
        script = new ScriptSquirrel("main.nut");
#endif

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

    // Destroy scripting subsystem
#ifdef SCRIPT_SUPPORT
#ifdef SCRIPT_SUPPORT
    script->update();
#endif
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

    // Note: This is just an idea, we could also pass the connection handler
    // to the constructor of the account handler, upon which is would register
    // itself for the messages it handles.
    //
    //AccountHandler *accountHandler = new AccountHandler();
    //connectionHandler->registerHandler(C2S_LOGIN, accountHandler);

    logger->log("The Mana World Server v%s", TMWSERV_VERSION);
    session->startListen(connectionHandler, SERVER_PORT);
    logger->log("Listening on port %d...", SERVER_PORT);

    SDL_Event event;

    delete script;
#endif
    delete logger;
}

/**
 * Update game world
 */
void updateWorld()
{
#ifdef SCRIPT_SUPPORT
    script->update();
#endif
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

    // Note: This is just an idea, we could also pass the connection handler
    // to the constructor of the account handler, upon which is would register
    // itself for the messages it handles.
    //
    //AccountHandler *accountHandler = new AccountHandler();
    //connectionHandler->registerHandler(C2S_LOGIN, accountHandler);

    logger->log("The Mana World Server v%s", TMWSERV_VERSION);
    session->startListen(connectionHandler, SERVER_PORT);
    logger->log("Listening on port %d...", SERVER_PORT);

    std::cout << "Number of accounts on server: " << store.accountCount() << std::endl;

    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == TMW_WORLD_TICK)
            {
                // Move the world forward in time
                worldTime++;

                // Print world time at 10 second intervals to show we're alive
                if (worldTime % 100 == 0) {
                    printf("World time: %d\n", worldTime);
                    logger->log("World time: %d", worldTime);
                }

                // - Handle all messages that are in the message queue
                // - Update all active objects/beings
                updateWorld();

            }
            else if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // We know only about 10 events will happen per second,
        // so give the CPU a break for a while.
        SDL_Delay(100);
    }

    logger->log("Recieved Quit signal, closing down...");
    session->stopListen(SERVER_PORT);

    deinitialize();
}
