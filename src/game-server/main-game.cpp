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
#include <iostream>
#include <signal.h>
#include <physfs.h>
#include <enet/enet.h>

#if (defined __USE_UNIX98 || defined __FreeBSD__)
#include "../config.h"
#endif

#include "configuration.h"
#include "resourcemanager.h"
#include "skill.h"
#include "game-server/accountconnection.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/state.hpp"
#include "net/connectionhandler.hpp"
#include "net/messageout.hpp"
#include "utils/logger.h"
#include "utils/stringfilter.h"
#include "utils/timer.h"

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
#define DEFAULT_ITEMSDB_FILE    "items.xml"
#define DEFAULT_MAPSDB_FILE     "maps.xml"

utils::Timer worldTimer(100, false);   /**< Timer for world tics set to 100 ms */
int worldTime = 0;              /**< Current world time in 100ms ticks */
bool running = true;            /**< Determines if server keeps running */

Skill skillTree("base");        /**< Skill tree */

Configuration config;           /**< XML config reader */

utils::StringFilter *stringFilter; /**< Slang's Filter */

/** Item manager */
ItemManager *itemManager;

/** Map manager */
MapManager *mapManager;

/** Core game message handler */
GameHandler *gameHandler;

/** Account server message handler */
AccountConnection *accountHandler;

/** Global game state */
State *gameState;

/** Callback used when SIGQUIT signal is received. */
void closeGracefully(int dummy)
{
    running = false;
}

/**
 * Initializes the server.
 */
void initialize()
{
    // Reset to default segmentation fault handling for debugging purposes
    signal(SIGSEGV, SIG_DFL);

    // Used to close via process signals
    signal(SIGQUIT, closeGracefully);
    signal(SIGINT, closeGracefully);

    /*
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

    // Initialize PhysicsFS
    PHYSFS_init("");

    // Initialize the logger.
    using namespace utils;
    Logger::setLogFile(logPath);

    // Write the messages to both the screen and the log file.
    Logger::setTeeMode(true);

    config.init(configPath);
    LOG_INFO("Using config file: " << configPath);
    LOG_INFO("Using log file: " << logPath);

    // --- Initialize the managers
    // Initialize the slang's and double quotes filter.
    stringFilter = new StringFilter(&config);
    // Initialize the map manager
    mapManager = new MapManager(DEFAULT_MAPSDB_FILE);
    // Initialize the item manager
    itemManager = new ItemManager(DEFAULT_ITEMSDB_FILE);

    // --- Initialize the global handlers
    // FIXME: Make the global handlers global vars or part of a bigger
    // singleton or a local variable in the event-loop
    gameHandler = new GameHandler();
    accountHandler = new AccountConnection();

    // --- Initialize enet.
    if (enet_initialize() != 0) {
        LOG_FATAL("An error occurred while initializing ENet");
        exit(2);
    }

    // Set enet to quit on exit.
    atexit(enet_deinitialize);

    // --- Initialize scripting subsystem.
#ifdef RUBY_SUPPORT
    LOG_INFO("Script language: " << scriptLanguage);

    // Initialize ruby
    ruby_init();
    ruby_init_loadpath();
    ruby_script("tmw");

    // Initialize bindings
    Init_Tmw();

    // Run test script
    rb_load_file("scripts/init.rb");
    rubyStatus = ruby_exec();
#else
    LOG_WARN("No scripting language support.");
#endif
}


/**
 * Deinitializes the server.
 */
void deinitialize()
{
    // Write configuration file
    config.write();

    // Stop world timer
    worldTimer.stop();

#ifdef RUBY_SUPPORT
    // Finish up ruby
    ruby_finalize();
    ruby_cleanup(rubyStatus);
#endif

    // Destroy message handlers
    delete gameHandler;
    delete accountHandler;

    // Destroy Managers
    delete stringFilter;
    delete itemManager;
    delete mapManager;

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
        { 0 }
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
                utils::Logger::setVerbosity(utils::Logger::Level(verbosityLevel));
                LOG_INFO("Setting log verbosity level to " << verbosityLevel);
                break;
            case 'p':
                // Change the port to listen on.
                unsigned short portToListenOn;
                portToListenOn = atoi(optarg);
                config.setValue("gameServerPort", portToListenOn);
                LOG_INFO("Setting default port to " << portToListenOn);
                break;
        }
    }
}


/**
 * Main function, initializes and runs server.
 */
int main(int argc, char *argv[])
{
    int elapsedWorldTicks;

    LOG_INFO("The Mana World Game Server v" << PACKAGE_VERSION);

    // Parse command line options
    parseOptions(argc, argv);

    // General initialization
    initialize();

    if (!accountHandler->start()) {
        LOG_FATAL("Unable to create a connection to an account server.");
        return 3;
    }

    int gameServerPort =
        (int) config.getValue("gameServerPort", DEFAULT_SERVER_PORT + 3);

    if (!gameHandler->startListen(gameServerPort))
    {
        LOG_FATAL("Unable to create an ENet server host.");
        return 3;
    }

    // Create state machine
    gameState = new State;

    // Initialize world timer
    worldTimer.start();

    while (running) {
        elapsedWorldTicks = worldTimer.poll();
        if (elapsedWorldTicks > 0) {
            worldTime += elapsedWorldTicks;

            if (elapsedWorldTicks > 1)
            {
                LOG_WARN(elapsedWorldTicks -1 << " World Tick(s) skipped "
                        "because of insufficient time. Please buy a faster "
                        "machine ;-)");
            };

            // Print world time at 10 second intervals to show we're alive
            if (worldTime % 100 == 0) {
                LOG_INFO("World time: " << worldTime);
            }

            // Handle all messages that are in the message queues
            accountHandler->process();
            gameHandler->process();
            // Update all active objects/beings
            gameState->update();
            // Send potentially urgent outgoing messages
            gameHandler->flush();
        }
        worldTimer.sleep();
    }

    LOG_INFO("Received: Quit signal, closing down...");
    gameHandler->stopListen();
    accountHandler->stop();
    delete gameState;
    deinitialize();
}
