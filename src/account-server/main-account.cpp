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
#include <fstream>
#include <physfs.h>
#include <enet/enet.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "account-server/accounthandler.hpp"
#include "account-server/guildmanager.hpp"
#include "account-server/serverhandler.hpp"
#include "account-server/dalstorage.hpp"
#include "chat-server/chatchannelmanager.hpp"
#include "chat-server/chathandler.hpp"
#include "common/configuration.hpp"
#include "net/connectionhandler.hpp"
#include "net/messageout.hpp"
#include "utils/logger.h"
#include "utils/processorutils.hpp"
#include "utils/stringfilter.h"
#include "utils/timer.h"

// Default options that automake should be able to override.
#define DEFAULT_LOG_FILE        "tmwserv-account.log"
#define DEFAULT_STATS_FILE      "tmwserv.stats"
#define DEFAULT_CONFIG_FILE     "tmwserv.xml"
#define DEFAULT_ITEMSDB_FILE    "items.xml"

static bool running = true;        /**< Determines if server keeps running */

utils::StringFilter *stringFilter; /**< Slang's Filter */

/** Database handler. */
DALStorage *storage;

/** Communications (chat) message handler */
ChatHandler *chatHandler;

/** Chat Channels Manager */
ChatChannelManager *chatChannelManager;

/** Guild Manager */
GuildManager *guildManager;

/** Callback used when SIGQUIT signal is received. */
static void closeGracefully(int)
{
    running = false;
}

/**
 * Initializes the server.
 */
static void initialize()
{

    // Reset to default segmentation fault handling for debugging purposes
    signal(SIGSEGV, SIG_DFL);

    // Used to close via process signals
#if (defined __USE_UNIX98 || defined __FreeBSD__)
    signal(SIGQUIT, closeGracefully);
#endif
    signal(SIGINT, closeGracefully);

    // Set enet to quit on exit.
    atexit(enet_deinitialize);

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

    // write the messages to both the screen and the log file.
    Logger::setTeeMode(true);

    Configuration::initialize(configPath);
    LOG_INFO("Using Config File: " << configPath);
    LOG_INFO("Using Log File: " << logPath);

    // Open database.
    storage = new DALStorage;
    storage->open();

    // --- Initialize the managers
    // Initialize the slang's and double quotes filter.
    stringFilter = new StringFilter;
    // Initialize the Chat channels manager
    chatChannelManager = new ChatChannelManager;

#if 0
    // Initialise the guild manager
    guildManager = new GuildManager;
#endif

    // --- Initialize the global handlers
    // FIXME: Make the global handlers global vars or part of a bigger
    // singleton or a local variable in the event-loop
    chatHandler = new ChatHandler;

    // --- Initialize enet.
    if (enet_initialize() != 0) {
        LOG_FATAL("An error occurred while initializing ENet");
        exit(2);
    }

    // Initialize the processor utility functions
    utils::processor::init();

    // Seed the random number generator
    std::srand( time(NULL) );
}


/**
 * Deinitializes the server.
 */
static void deinitialize()
{
    delete stringFilter;
    // Write configuration file
    Configuration::deinitialize();

    // Destroy message handlers.
    AccountClientHandler::deinitialize();
    GameServerHandler::deinitialize();

    // Quit ENet
    enet_deinitialize();

    delete chatHandler;

    // Destroy Managers
    delete chatChannelManager;
#if 0
    delete guildManager;
#endif

    // Get rid of persistent data storage
    delete storage;

    PHYSFS_deinit();
}

/**
 * Dumps statistics.
 */
static void dumpStatistics()
{
#if defined STATS_FILE
    std::string path = STATS_FILE;
#else

#if (defined __USE_UNIX98 || defined __FreeBSD__)
    std::string path = getenv("HOME");
    path += "/.";
    path += DEFAULT_STATS_FILE;
#else // Win32, ...
    std::string path = DEFAULT_STATS_FILE;
#endif

#endif

    std::ofstream os(path.c_str());
    os << "<statistics>\n";
    GameServerHandler::dumpStatistics(os);
    os << "</statistics>\n";
}

/**
 * Show command line arguments
 */
static void printHelp()
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
static void parseOptions(int argc, char *argv[])
{
    const char *optstring = "h";

    const struct option long_options[] = {
        { "help",       no_argument, 0, 'h' },
        { "verbosity",  required_argument, 0, 'v' },
        { "port",       required_argument, 0, 'p' },
        { 0, 0, 0, 0 }
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
                LOG_INFO("Setting Log Verbosity Level to " << verbosityLevel);
                break;
            case 'p':
                // Change the port to listen on.
                unsigned short portToListenOn;
                portToListenOn = atoi(optarg);
                Configuration::setValue("ListenOnPort", portToListenOn);
                LOG_INFO("Setting Default Port to " << portToListenOn);
                break;
        }
    }
}


/**
 * Main function, initializes and runs server.
 */
int main(int argc, char *argv[])
{
    LOG_INFO("The Mana World Account+Chat Server v" << PACKAGE_VERSION);

    // Parse Command Line Options
    parseOptions(argc, argv);

    // General Initialization
    initialize();

    int port = Configuration::getValue("accountServerPort", DEFAULT_SERVER_PORT);
    if (!AccountClientHandler::initialize(port) ||
        !GameServerHandler::initialize(port + 1) ||
        !chatHandler->startListen(port + 2))
    {
        LOG_FATAL("Unable to create an ENet server host.");
        return 3;
    }

    // Dump statistics every 10 seconds.
    utils::Timer statTimer(10000);

    while (running) {
        AccountClientHandler::process();
        GameServerHandler::process();
        chatHandler->process(50);
        if (statTimer.poll()) dumpStatistics();
    }

    LOG_INFO("Received: Quit signal, closing down...");
    chatHandler->stopListen();
    deinitialize();
}
