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
#include <enet/enet.h>

#if (defined __USE_UNIX98 || defined __FreeBSD__)
#include "../config.h"
#elif defined WIN32
#include "../tmwserv_private.h"
#define PACKAGE_VERSION PRODUCT_VERSION
#endif

#include "accounthandler.h"
#include "chatchannelmanager.h"
#include "chathandler.h"
#include "configuration.h"
#include "connectionhandler.h"
#include "gamehandler.h"
#include "messageout.h"
#include "resourcemanager.h"
#include "skill.h"
#include "state.h"
#include "storage.h"

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
#ifndef DEFAULT_SERVER_PORT
#define DEFAULT_SERVER_PORT     9601
#endif

utils::Timer worldTimer(100, false);   /**< Timer for world tics set to 100 ms */
int worldTime = 0;              /**< Current world time in 100ms ticks */
bool running = true;            /**< Determines if server keeps running */

Skill skillTree("base");        /**< Skill tree */

Configuration config;           /**< XML config reader */

utils::StringFilter *stringFilter; /**< Slang's Filter */

/** Account message handler */
AccountHandler *accountHandler;

/** Communications (chat) message handler */
ChatHandler *chatHandler;
/** Chat Channels Manager */
ChatChannelManager *chatChannelManager;

/** Core game message handler */
GameHandler *gameHandler;

/** Global game state */
State *gameState;

/**
 * Initializes the server.
 */
void initialize()
{
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

    // initialize the logger.
    using namespace utils;
    Logger::instance().setLogFile(logPath);

    // write the messages to both the screen and the log file.
    Logger::instance().setTeeMode(true);

    config.init(configPath);
    LOG_INFO("Using Config File: " << configPath, 0);
    LOG_INFO("Using Log File: " << logPath, 0);

    // Initialize the slang's filter.
    stringFilter = new StringFilter(&config);

    // Initialize the global handlers
    // FIXME: Make the global handlers global vars or part of a bigger
    // singleton or a local variable in the event-loop
    chatChannelManager = new ChatChannelManager();

    chatHandler = new ChatHandler();
    accountHandler = new AccountHandler();
    gameHandler = new GameHandler();

    // Reset to default segmentation fault handling for debugging purposes
    signal(SIGSEGV, SIG_DFL);

    // Set enet to quit on exit.
    atexit(enet_deinitialize);

    // Initialize enet.
    if (enet_initialize() != 0) {
        LOG_FATAL("An error occurred while initializing ENet", 0);
        exit(2);
    }

    // Initialize scripting subsystem.
#ifdef RUBY_SUPPORT
    LOG_INFO("Script Language: " << scriptLanguage, 0);

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
    LOG_WARN("No Scripting Language Support.", 0);
#endif

#if defined (MYSQL_SUPPORT)
    LOG_INFO("Using MySQL DB Backend.", 0);
#elif defined (POSTGRESQL_SUPPORT)
    LOG_INFO("Using PostGreSQL DB Backend.", 0);
#elif defined (SQLITE_SUPPORT)
    LOG_INFO("Using SQLite DB Backend.", 0);
#else
    LOG_WARN("No Database Backend Support.", 0);
#endif

    // Initialize configuration defaults
    config.setValue("dbuser", "");
    config.setValue("dbpass", "");
    config.setValue("dbhost", "");

    // Initialize PhysicsFS
    PHYSFS_init("");
}


/**
 * Deinitializes the server.
 */
void deinitialize()
{
    delete stringFilter;
    // Write configuration file
    config.write();

    // Stop world timer
    worldTimer.stop();

    // Quit ENet
    enet_deinitialize();

#ifdef RUBY_SUPPORT
    // Finish up ruby
    ruby_finalize();
    ruby_cleanup(rubyStatus);
#endif

    // Destroy message handlers
    delete accountHandler;
    delete chatHandler;
    delete gameHandler;

    delete chatChannelManager;

    // Get rid of persistent data storage
    Storage::destroy();

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
                utils::Logger::instance().setVerbosity(verbosityLevel);
                LOG_INFO("Setting Log Verbosity Level to " << verbosityLevel, 0);
                break;
            case 'p':
                // Change the port to listen on.
                unsigned short portToListenOn;
                portToListenOn = atoi(optarg);
                config.setValue("ListenOnPort", portToListenOn);
                LOG_INFO("Setting Default Port to " << portToListenOn, 0);
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

    LOG_INFO("The Mana World Server v" << PACKAGE_VERSION, 0);

    // Parse Command Line Options
    parseOptions(argc, argv);

    // General Initialization
    initialize();

    if (!accountHandler->startListen(int(config.getValue("ListenOnPort", DEFAULT_SERVER_PORT))) ||
        !chatHandler->startListen(int(config.getValue("ListenOnPort", DEFAULT_SERVER_PORT)) + 1) ||
        !gameHandler->startListen(int(config.getValue("ListenOnPort", DEFAULT_SERVER_PORT)) + 2)) {
        LOG_ERROR("Unable to create an ENet server host.", 0);
        return 3;
    }

    // Create storage wrapper
    Storage& store = Storage::instance("tmw");
    store.setUser(config.getValue("dbuser", ""));
    store.setPassword(config.getValue("dbpass", ""));
    store.close();
    store.open();

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
                        "because of insufficient time. please buy a faster "
                        "machine ;-)", 0);
            };

            // Print world time at 10 second intervals to show we're alive
            if (worldTime % 100 == 0) {
                LOG_INFO("World time: " << worldTime, 0);
            }

            // Handle all messages that are in the message queues
            accountHandler->process();
            chatHandler->process();
            gameHandler->process();
            // Update all active objects/beings
            gameState->update();
            // Send potentially urgent outgoing messages
            gameHandler->flush();
        }
        worldTimer.sleep();
    }

    LOG_INFO("Received: Quit signal, closing down...", 0);
    gameHandler->stopListen();
    delete gameState;
    chatHandler->stopListen();
    accountHandler->stopListen();
    deinitialize();
}
