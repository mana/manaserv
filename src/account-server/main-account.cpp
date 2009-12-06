/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
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
#include "account-server/serverhandler.hpp"
#include "account-server/storage.hpp"
#include "chat-server/chatchannelmanager.hpp"
#include "chat-server/chathandler.hpp"
#include "chat-server/guildmanager.hpp"
#include "chat-server/post.hpp"
#include "common/configuration.hpp"
#include "net/bandwidth.hpp"
#include "net/connectionhandler.hpp"
#include "net/messageout.hpp"
#include "utils/logger.h"
#include "utils/processorutils.hpp"
#include "utils/stringfilter.h"
#include "utils/timer.h"

using utils::Logger;

// Default options that automake should be able to override.
#define DEFAULT_LOG_FILE        "manaserv-account.log"
#define DEFAULT_STATS_FILE      "manaserv.stats"
#define DEFAULT_CONFIG_FILE     "manaserv.xml"

static bool running = true;        /**< Determines if server keeps running */

utils::StringFilter *stringFilter; /**< Slang's Filter */

/** Database handler. */
Storage *storage;

/** Communications (chat) message handler */
ChatHandler *chatHandler;

/** Chat Channels Manager */
ChatChannelManager *chatChannelManager;

/** Guild Manager */
GuildManager *guildManager;

/** Post Manager */
PostManager *postalManager;

/** Bandwidth Monitor */
BandwidthMonitor *gBandwidth;

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
    signal(SIGTERM, closeGracefully);

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
    LOG_INFO("Using config file: " << configPath);
    LOG_INFO("Using log file: " << logPath);

    // check inter-server password
    if (Configuration::getValue("net_password", "") == "")
    {
        LOG_WARN("SECURITY WARNING: No net_password set in " << configPath <<
                 " - set one ASAP or this server WILL get h4x0rd!!");
    }

    // Open database
    try {
        storage = new Storage;
        storage->open();
    } catch (std::string &error) {
        LOG_FATAL("Error opening the database: " << error);
        exit(1);
    }

    // --- Initialize the managers
    // Initialize the slang's and double quotes filter.
    stringFilter = new StringFilter;
    // Initialize the Chat channels manager
    chatChannelManager = new ChatChannelManager;
    // Initialise the Guild manager
    guildManager = new GuildManager;
    // Initialise the post manager
    postalManager = new PostManager;
    // Initialise the bandwidth monitor
    gBandwidth = new BandwidthMonitor;

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
    // Write configuration file
    Configuration::deinitialize();

    // Destroy message handlers.
    AccountClientHandler::deinitialize();
    GameServerHandler::deinitialize();

    // Quit ENet
    enet_deinitialize();

    delete chatHandler;

    // Destroy Managers
    delete stringFilter;
    delete chatChannelManager;
    delete guildManager;
    delete postalManager;
    delete gBandwidth;

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
    std::cout << "manaserv" << std::endl << std::endl
              << "Options: " << std::endl
              << "  -h --help          : Display this help" << std::endl
              << "     --verbosity <n> : Set the verbosity level" << std::endl
              << "     --port <n>      : Set the default port to listen on" << std::endl;
    exit(0);
}

struct CommandLineOptions
{
    CommandLineOptions():
        verbosity(Logger::INFO),
        port(Configuration::getValue("net_accountServerPort",
                                     DEFAULT_SERVER_PORT))
    {}

    Logger::Level verbosity;
    int port;
};

/**
 * Parse the command line arguments
 */
static void parseOptions(int argc, char *argv[], CommandLineOptions &options)
{
    const char *optstring = "h";

    const struct option long_options[] =
    {
        { "help",       no_argument, 0, 'h' },
        { "verbosity",  required_argument, 0, 'v' },
        { "port",       required_argument, 0, 'p' },
        { 0, 0, 0, 0 }
    };

    while (optind < argc)
    {
        int result = getopt_long(argc, argv, optstring, long_options, NULL);

        if (result == -1)
            break;

        switch (result) {
            default: // Unknown option
            case 'h':
                // Print help
                printHelp();
                break;
            case 'v':
                options.verbosity = static_cast<Logger::Level>(atoi(optarg));
                LOG_INFO("Using log verbosity level " << options.verbosity);
                break;
            case 'p':
                options.port = atoi(optarg);
                break;
        }
    }
}


/**
 * Main function, initializes and runs server.
 */
int main(int argc, char *argv[])
{
#ifdef PACKAGE_VERSION
    LOG_INFO("The Mana Account+Chat Server v" << PACKAGE_VERSION);
#endif

    // Parse command line options
    CommandLineOptions options;
    parseOptions(argc, argv, options);
    Logger::setVerbosity(options.verbosity);

    // General initialization
    initialize();

    std::string host = Configuration::getValue("net_listenHost", std::string());
    if (!AccountClientHandler::initialize(options.port, host) ||
        !GameServerHandler::initialize(options.port + 1, host) ||
        !chatHandler->startListen(options.port + 2, host))
    {
        LOG_FATAL("Unable to create an ENet server host.");
        return 3;
    }

    // Dump statistics every 10 seconds.
    utils::Timer statTimer(10000);
    // Check for expired bans every 30 seconds
    utils::Timer banTimer(30000);

    // -------------------------------------------------------------------------
    // FIXME: for testing purposes only...
    // writing accountserver startup time and svn revision to database as global
    // world state variable
    const time_t startup = time(NULL);
    std::stringstream timestamp;
    timestamp << startup;
    storage->setWorldStateVar("accountserver_startup", timestamp.str());
    const std::string revision = "$Revision$";
    storage->setWorldStateVar("accountserver_version", revision);
    // -------------------------------------------------------------------------

    while (running)
    {
        AccountClientHandler::process();
        GameServerHandler::process();
        chatHandler->process(50);

        if (statTimer.poll())
            dumpStatistics();

        if (banTimer.poll())
            storage->checkBannedAccounts();
    }

    LOG_INFO("Received: Quit signal, closing down...");
    chatHandler->stopListen();
    deinitialize();

    return 0;
}
