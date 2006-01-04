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

#include <cstdlib>
#include <getopt.h>
#include <signal.h>
#include <iostream>
#include <physfs.h>
#include <SDL.h>
#include <SDL_net.h>

#if (defined __USE_UNIX98 || defined __FreeBSD__)
#include "../config.h"
#endif

#include "accounthandler.h"
#include "chathandler.h"
#include "configuration.h"
#include "connectionhandler.h"
#include "gamehandler.h"
#include "mapmanager.h"
#include "netsession.h"
#include "resourcemanager.h"
#include "skill.h"
#include "state.h"
#include "storage.h"

#include "utils/logger.h"
#include "utils/slangsfilter.h"

// Scripting
#ifdef SCRIPT_SUPPORT

extern "C" void Init_Tmw();

#if defined (SQUIRREL_SUPPORT)
std::string scriptLanguage = "squirrel";
#elif defined (RUBY_SUPPORT)
#include <ruby.h>
int rubyStatus;
std::string scriptLanguage = "ruby";
#elif defined (LUA_SUPPORT)
std::string scriptLanguage = "lua";
#else
#error "Scripting enabled, but no language selected"
#endif

#else
std::string scriptLanugage = "none";
#endif // SCRIPT_SUPPORT

// Default options that automake should be able to override.
#define DEFAULT_LOG_FILE        "tmwserv.log"
#define DEFAULT_CONFIG_FILE     "tmwserv.xml"
#ifndef DEFAULT_SERVER_PORT
#define DEFAULT_SERVER_PORT     9601
#endif

#define TMW_WORLD_TICK  SDL_USEREVENT

SDL_TimerID worldTimerID; /**< Timer ID of world timer */
int worldTime = 0;        /**< Current world time in 100ms ticks */
bool running = true;      /**< Determines if server keeps running */

Skill skillTree("base");  /**< Skill tree */

Configuration config;     /**< XML config reader */

tmwserv::utils::SlangsFilter *slangsFilter; /**< Slang's Filter */

/** Account message handler */
AccountHandler *accountHandler;

/** Communications (chat) message handler */
ChatHandler *chatHandler;

/** Core game message handler */
GameHandler *gameHandler;

/** Primary connection handler */
ConnectionHandler *connectionHandler;

/**
 * SDL timer callback, sends a <code>TMW_WORLD_TICK</code> event.
 */
Uint32 worldTick(Uint32 interval, void *param)
{
    // Push the custom world tick event
    SDL_Event event;
    event.type = TMW_WORLD_TICK;

    if (SDL_PushEvent(&event)) {
        LOG_WARN("couldn't push world tick into event queue!", 0)
    }

    return interval;
}


/**
 * Initializes the server.
 */
void initialize()
{

/**
 * If the path values aren't defined, we set the default
 * depending on the platform.
 */
// The config path
#if defined CONFIG_FILE
    std::string configPath = CONFIG_FILE;
#else

#if (defined __USE_UNIX98 || defined __FreeBSD__)
    std::string configPath = getenv("HOME");
    configPath += "/.";
    configPath += DEFAULT_CONFIG_FILE;
#else // Win32, ...
    std::string configPath = DEFAULT_CONFIG_FILE;
#endif

#endif // defined CONFIG_FILE

// The log path
#if defined LOG_FILE
    std::string logPath = LOG_FILE;
#else

#if (defined __USE_UNIX98 || defined __FreeBSD__)
    std::string logPath = getenv("HOME");
    logPath += "/.";
    logPath += DEFAULT_LOG_FILE;
#else // Win32, ...
    std::string logPath = DEFAULT_LOG_FILE;
#endif

#endif // defined LOG_FILE

    // initialize the logger.
    using namespace tmwserv::utils;
    Logger::instance().setLogFile(logPath);

    // write the messages to both the screen and the log file.
    Logger::instance().setTeeMode(true);

    config.init(configPath);
    LOG_INFO("Using Config File: " << configPath, 0)
    LOG_INFO("Using Log File: " << logPath, 0)

    // Initialize the slang's filter.
    slangsFilter = new SlangsFilter(&config);

    // Initialize the global handlers
    // FIXME: Make the global handlers global vars or part of a bigger
    // singleton or a local vatiable in the event-loop
    accountHandler = new AccountHandler();
    chatHandler = new ChatHandler();
    gameHandler = new GameHandler();
    connectionHandler = new ConnectionHandler();

    // Make SDL use a dummy videodriver so that it doesn't require an X server
    putenv("SDL_VIDEODRIVER=dummy");

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
        LOG_FATAL("SDL_Init: " << SDL_GetError(), 0)
        exit(1);
    }

    signal(SIGSEGV, SIG_DFL);

    // set SDL to quit on exit.
    atexit(SDL_Quit);

    // initialize SDL_net.
    if (SDLNet_Init() == -1) {
        LOG_FATAL("SDLNet_Init: " << SDLNet_GetError(), 0)
        exit(2);
    }

    // initialize world timer at 10 times per second.
    worldTimerID = SDL_AddTimer(100, worldTick, NULL);

    // initialize scripting subsystem.
#ifdef RUBY_SUPPORT
    LOG_INFO("Script Language: " << scriptLanguage, 0)

    // initialize ruby
    ruby_init();
    ruby_init_loadpath();
    ruby_script("tmw");

    // initialize bindings
    Init_Tmw();

    // run test script
    rb_load_file("scripts/init.rb");
    rubyStatus = ruby_exec();
#else
    LOG_WARN("No Scripting Language Support.", 0)
#endif

#if defined (MYSQL_SUPPORT)
    LOG_INFO("Using MySQL DB Backend.", 0)
#elif defined (POSTGRESQL_SUPPORT)
    LOG_INFO("Using PostGreSQL DB Backend.", 0)
#elif defined (SQLITE_SUPPORT)
    LOG_INFO("Using SQLite DB Backend.", 0)
#else
    LOG_WARN("No Database Backend Support.", 0)
#endif

    // initialize configuration defaults
    config.setValue("dbuser", "");
    config.setValue("dbpass", "");
    config.setValue("dbhost", "");

    // Initialize PhysicsFS
    PHYSFS_init("");

    // TODO: only a test, maps should be loaded as they are needed
    tmwserv::MapManager::instance().loadMap("tulimshar.tmx.gz");
    tmwserv::MapManager::instance().reloadMap("tulimshar.tmx.gz");
    tmwserv::MapManager::instance().unloadMap("tulimshar.tmx.gz");
}


/**
 * Deinitializes the server.
 */
void deinitialize()
{
    delete slangsFilter;
    // Write configuration file
    config.write();

    // Stop world timer
    SDL_RemoveTimer(worldTimerID);

    // Quit SDL_net
    SDLNet_Quit();

#ifdef RUBY_SUPPORT
    // Finish up ruby
    ruby_finalize();
    ruby_cleanup(rubyStatus);
#endif

    // destroy message handlers
    delete accountHandler;
    delete chatHandler;
    delete gameHandler;
    delete connectionHandler;

    // Get rid of persistent data storage
    tmwserv::Storage::destroy();

    PHYSFS_deinit();
}


/**
 * Show command line arguments
 */
void printHelp()
{
    std::cout << "tmwserv" << std::endl << std::endl
              << "Options: " << std::endl
              << "  -h --help          : Display this help" << std::endl
              << "     --verbosity <n> : Set the verbosity level" << std::endl
              << "     --port <n>      : Set the default port to listen on" << std::endl;
    exit(0);
}

/**
 * Parse the command line arguments
 */
void parseOptions(int argc, char *argv[])
{
    const char *optstring = "h";

    const struct option long_options[] = {
        { "help",       no_argument, 0, 'h' },
        { "verbosity",  required_argument, 0, 'v' },
        { "port",       required_argument, 0, 'p' },
        0
    };

    while (optind < argc) {
        int result = getopt_long(argc, argv, optstring, long_options, NULL);

        if (result == -1) {
            break;
        }

        switch (result) {
            default: // Unknown option
            case 'h':
                // Print help
                printHelp();
                break;
            case 'v':
                // Set Verbosity to level
                unsigned short verbosityLevel;
                verbosityLevel = atoi(optarg);
                tmwserv::utils::Logger::instance().setVerbosity(verbosityLevel);
                LOG_INFO("Setting Log Verbosity Level to " << verbosityLevel, 0)
                break;
            case 'p':
                // Change the port to listen on.
                unsigned short portToListenOn;
                portToListenOn = atoi(optarg);
                config.setValue("ListenOnPort", portToListenOn);
                LOG_INFO("Setting Default Port to " << portToListenOn, 0)
                break;
        }
    }
}


/**
 * Main function, initializes and runs server.
 */
int main(int argc, char *argv[])
{
#if (defined __USE_UNIX98 || defined __FreeBSD__)
    LOG_INFO("The Mana World Server v" << PACKAGE_VERSION, 0)
#endif

    // Parse Command Line Options
    parseOptions(argc, argv);

    // General Initialization
    initialize();

    // Ready for server work...
    std::auto_ptr<NetSession> session(new NetSession());

    // Note: This is just an idea, we could also pass the connection handler
    // to the constructor of the account handler, upon which is would register
    // itself for the messages it handles.
    //

    // Register message handlers
    connectionHandler->registerHandler(CMSG_LOGIN, accountHandler);
    connectionHandler->registerHandler(CMSG_LOGOUT, accountHandler);
    connectionHandler->registerHandler(CMSG_REGISTER, accountHandler);
    connectionHandler->registerHandler(CMSG_UNREGISTER, accountHandler);
    connectionHandler->registerHandler(CMSG_CHAR_CREATE, accountHandler);
    connectionHandler->registerHandler(CMSG_CHAR_SELECT, accountHandler);
    connectionHandler->registerHandler(CMSG_CHAR_DELETE, accountHandler);
    connectionHandler->registerHandler(CMSG_CHAR_LIST, accountHandler);
    connectionHandler->registerHandler(CMSG_EMAIL_GET, accountHandler);
    connectionHandler->registerHandler(CMSG_PASSWORD_CHANGE, accountHandler);
    connectionHandler->registerHandler(CMSG_EMAIL_CHANGE, accountHandler);

    connectionHandler->registerHandler(CMSG_SAY, chatHandler);
    connectionHandler->registerHandler(CMSG_ANNOUNCE, chatHandler);

    connectionHandler->registerHandler(CMSG_PICKUP, gameHandler);
    connectionHandler->registerHandler(CMSG_USE_OBJECT, gameHandler);
    connectionHandler->registerHandler(CMSG_USE_ITEM, gameHandler); // NOTE: this is probably redundant (CMSG_USE_OBJECT)
    connectionHandler->registerHandler(CMSG_TARGET, gameHandler);
    connectionHandler->registerHandler(CMSG_WALK, gameHandler);
    connectionHandler->registerHandler(CMSG_START_TRADE, gameHandler);
    connectionHandler->registerHandler(CMSG_START_TALK, gameHandler);
    connectionHandler->registerHandler(CMSG_REQ_TRADE, gameHandler);
    connectionHandler->registerHandler(CMSG_EQUIP, gameHandler);

    session->startListen(connectionHandler, int(config.getValue("ListenOnPort", DEFAULT_SERVER_PORT)));
    LOG_INFO("Listening on port " << config.getValue("ListenOnPort", DEFAULT_SERVER_PORT) << "...", 0)

    using namespace tmwserv;

    // create storage wrapper
    Storage& store = Storage::instance("tmw");
    store.setUser(config.getValue("dbuser", ""));
    store.setPassword(config.getValue("dbpass", ""));
    store.close();
    store.open();
    //

    // create state machine
    State &state = State::instance();
    //

    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == TMW_WORLD_TICK) {
                // Move the world forward in time
                worldTime++;

                // Print world time at 10 second intervals to show we're alive
                if (worldTime % 100 == 0) {
                    LOG_INFO("World time: " << worldTime, 0);
                }

                // - Handle all messages that are in the message queue
                // - Update all active objects/beings
                state.update(*connectionHandler);
            }
            else if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // We know only about 10 events will happen per second,
        // so give the CPU a break for a while.
        SDL_Delay(100);
    }

    LOG_INFO("Received: Quit signal, closing down...", 0)
    session->stopListen(int(config.getValue("ListenOnPort", DEFAULT_SERVER_PORT)));

    deinitialize();
}
