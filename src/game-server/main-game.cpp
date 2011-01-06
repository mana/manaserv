/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *  Copyright (C) 2010  The Mana Developers
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
#include <iostream>
#include <signal.h>
#include <physfs.h>
#include <enet/enet.h>
#include <unistd.h>

#ifdef __MINGW32__
#include <windows.h>
#define usleep(usec) (Sleep ((usec) / 1000), 0)
#endif

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "common/configuration.h"
#include "common/permissionmanager.h"
#include "common/resourcemanager.h"
#include "game-server/accountconnection.h"
#include "game-server/attributemanager.h"
#include "game-server/gamehandler.h"
#include "game-server/skillmanager.h"
#include "game-server/itemmanager.h"
#include "game-server/mapmanager.h"
#include "game-server/monstermanager.h"
#include "game-server/statusmanager.h"
#include "game-server/postman.h"
#include "game-server/state.h"
#include "net/bandwidth.h"
#include "net/connectionhandler.h"
#include "net/messageout.h"
#include "scripting/luascript.h"
#include "utils/logger.h"
#include "utils/processorutils.h"
#include "utils/stringfilter.h"
#include "utils/timer.h"
#include "utils/mathutils.h"

using utils::Logger;

// Default options that automake should be able to override.
#define DEFAULT_LOG_FILE                    "manaserv-game.log"
#define DEFAULT_CONFIG_FILE                 "manaserv.xml"
#define DEFAULT_ITEMSDB_FILE                "items.xml"
#define DEFAULT_EQUIPDB_FILE                "equip.xml"
#define DEFAULT_SKILLSDB_FILE               "mana-skills.xml"
#define DEFAULT_ATTRIBUTEDB_FILE            "attributes.xml"
#define DEFAULT_MAPSDB_FILE                 "maps.xml"
#define DEFAULT_MONSTERSDB_FILE             "monsters.xml"
#define DEFAULT_STATUSDB_FILE               "mana-status-effect.xml"
#define DEFAULT_PERMISSION_FILE             "permissions.xml"
#define DEFAULT_GLOBAL_EVENT_SCRIPT_FILE    "scripts/global_events.lua"
#define DEFAULT_SPECIAL_ACTIONS_SCRIPT_FILE "scripts/special_actions.lua"

static int const WORLD_TICK_SKIP = 2; /** tolerance for lagging behind in world calculation) **/

/** Timer for world ticks */
utils::Timer worldTimer(WORLD_TICK_MS, false);
int worldTime = 0;              /**< Current world time in ticks */
bool running = true;            /**< Determines if server keeps running */

utils::StringFilter *stringFilter; /**< Slang's Filter */

AttributeManager *attributeManager = new AttributeManager(DEFAULT_ATTRIBUTEDB_FILE);
ItemManager *itemManager = new ItemManager(DEFAULT_ITEMSDB_FILE, DEFAULT_EQUIPDB_FILE);
MonsterManager *monsterManager = new MonsterManager(DEFAULT_MONSTERSDB_FILE);

/** Core game message handler */
GameHandler *gameHandler;

/** Account server message handler */
AccountConnection *accountHandler;

/** Post Man **/
PostMan *postMan;

/** Bandwidth Monitor */
BandwidthMonitor *gBandwidth;

/** Callback used when SIGQUIT signal is received. */
static void closeGracefully(int)
{
    running = false;
}

static void initializeConfiguration(std::string configPath = std::string())
{
    if (configPath.empty())
        configPath = DEFAULT_CONFIG_FILE;

    bool configFound = true;
    if (!Configuration::initialize(configPath))
    {
        configFound = false;

        // If the config file isn't the default and fail to load,
        // we try the default one with a warning.
        if (configPath.compare(DEFAULT_CONFIG_FILE))
        {
            LOG_WARN("Invalid config path: " << configPath
                     << ". Trying default value: " << DEFAULT_CONFIG_FILE ".");
            configPath = DEFAULT_CONFIG_FILE;
            configFound = true;

            if (!Configuration::initialize(configPath))
                  configFound = false;
        }

        if (!configFound)
        {
            LOG_FATAL("Refusing to run without configuration!" << std::endl
            << "Invalid config path: " << configPath << ".");
            exit(EXIT_CONFIG_NOT_FOUND);
        }
    }

    LOG_INFO("Using config file: " << configPath);

    // Check inter-server password.
    if (Configuration::getValue("net_password", "") == "")
    {
        LOG_FATAL("SECURITY WARNING: 'net_password' not set!");
        exit(EXIT_BAD_CONFIG_PARAMETER);
    }
}

static void initializeServer()
{
    // Reset to default segmentation fault handling for debugging purposes
    signal(SIGSEGV, SIG_DFL);

    // Used to close via process signals
#if (defined __USE_UNIX98 || defined __FreeBSD__)
    signal(SIGQUIT, closeGracefully);
#endif
    signal(SIGINT, closeGracefully);
    signal(SIGTERM, closeGracefully);

    std::string logFile = Configuration::getValue("log_gameServerFile",
                                                  DEFAULT_LOG_FILE);

    // Initialize PhysicsFS
    PHYSFS_init("");

    // Initialize the logger.
    Logger::setLogFile(logFile, true);

    // Write the messages to both the screen and the log file.
    Logger::setTeeMode(Configuration::getBoolValue("log_gameToStandardOutput",
                                                   true));

    LOG_INFO("Using log file: " << logFile);

    // Set up the options related to log rotation.
    Logger::enableLogRotation(Configuration::getBoolValue("log_enableRotation",
                                                          false));

    Logger::setMaxLogfileSize(Configuration::getValue("log_maxFileSize",
                                                      1024));

    Logger::setSwitchLogEachDay(Configuration::getBoolValue("log_perDay",
                                                            false));

    // --- Initialize the managers
    // Initialize the slang's and double quotes filter.
    stringFilter = new utils::StringFilter;

    ResourceManager::initialize();
    if (MapManager::initialize(DEFAULT_MAPSDB_FILE) < 1)
    {
        LOG_FATAL("The Game Server can't find any valid/available maps.");
        exit(EXIT_MAP_FILE_NOT_FOUND);
    }
    attributeManager->initialize();
    SkillManager::initialize(DEFAULT_SKILLSDB_FILE);
    itemManager->initialize();
    monsterManager->initialize();
    StatusManager::initialize(DEFAULT_STATUSDB_FILE);
    PermissionManager::initialize(DEFAULT_PERMISSION_FILE);

    LuaScript::loadGlobalEventScript(DEFAULT_GLOBAL_EVENT_SCRIPT_FILE);
    LuaScript::loadSpecialActionsScript(DEFAULT_SPECIAL_ACTIONS_SCRIPT_FILE);

    // --- Initialize the global handlers
    // FIXME: Make the global handlers global vars or part of a bigger
    // singleton or a local variable in the event-loop
    gameHandler = new GameHandler;
    accountHandler = new AccountConnection;
    postMan = new PostMan;
    gBandwidth = new BandwidthMonitor;

    // --- Initialize enet.
    if (enet_initialize() != 0)
    {
        LOG_FATAL("An error occurred while initializing ENet");
        exit(EXIT_NET_EXCEPTION);
    }

    // Set enet to quit on exit.
    atexit(enet_deinitialize);

    // Pre-calculate the needed trigomic function values
    utils::math::init();

    // Initialize the processor utility functions
    utils::processor::init();

    // Seed the random number generator
    std::srand( time(NULL) );
}


static void deinitializeServer()
{
    // Write configuration file
    Configuration::deinitialize();

    // Stop world timer
    worldTimer.stop();

    // Destroy message handlers
    delete gameHandler;
    delete accountHandler;
    delete postMan;
    delete gBandwidth;

    // Destroy Managers
    delete stringFilter;
    monsterManager->deinitialize();
    itemManager->deinitialize();
    MapManager::deinitialize();
    StatusManager::deinitialize();

    PHYSFS_deinit();
}


/**
 * Show command line arguments.
 */
static void printHelp()
{
    std::cout << "manaserv" << std::endl << std::endl
              << "Options: " << std::endl
              << "  -h --help          : Display this help" << std::endl
              << "     --config <path> : Set the config path to use."
              << " (Default: ./manaserv.xml)" << std::endl
              << "     --verbosity <n> : Set the verbosity level" << std::endl
              << "                        - 0. Fatal Errors only." << std::endl
              << "                        - 1. All Errors." << std::endl
              << "                        - 2. Plus warnings." << std::endl
              << "                        - 3. Plus standard information." << std::endl
              << "                        - 4. Plus debugging information." << std::endl
              << "     --port <n>      : Set the default port to listen on."
              << std::endl;
    exit(EXIT_NORMAL);
}

struct CommandLineOptions
{
    CommandLineOptions():
        configPath(DEFAULT_CONFIG_FILE),
        configPathChanged(false),
        verbosity(Logger::Warn),
        verbosityChanged(false),
        port(DEFAULT_SERVER_PORT + 3),
        portChanged(false)
    {}

    std::string configPath;
    bool configPathChanged;

    Logger::Level verbosity;
    bool verbosityChanged;

    int port;
    bool portChanged;
};

/**
 * Parse the command line arguments
 */
static void parseOptions(int argc, char *argv[], CommandLineOptions &options)
{
    const char *optString = "h";

    const struct option longOptions[] =
    {
        { "help",       no_argument,       0, 'h' },
        { "config",     required_argument, 0, 'c' },
        { "verbosity",  required_argument, 0, 'v' },
        { "port",       required_argument, 0, 'p' },
        { 0, 0, 0, 0 }
    };

    while (optind < argc)
    {
        int result = getopt_long(argc, argv, optString, longOptions, NULL);

        if (result == -1)
            break;

        switch (result)
        {
            default: // Unknown option.
            case 'h':
                // Print help.
                printHelp();
                break;
            case 'c':
                // Change config filename and path.
                options.configPath = optarg;
                options.configPathChanged = true;
                break;
            case 'v':
                options.verbosity = static_cast<Logger::Level>(atoi(optarg));
                options.verbosityChanged = true;
                LOG_INFO("Using log verbosity level " << options.verbosity);
                break;
            case 'p':
                options.port = atoi(optarg);
                options.portChanged = true;
                break;
        }
    }
}


/**
 * Main function, initializes and runs server.
 */
int main(int argc, char *argv[])
{
    // Parse command line options
    CommandLineOptions options;
    parseOptions(argc, argv, options);

    initializeConfiguration(options.configPath);

    // General initialization
    initializeServer();

#ifdef PACKAGE_VERSION
    LOG_INFO("The Mana Game Server v" << PACKAGE_VERSION);
#else
    LOG_INFO("The Mana Game Server (unknown version)");
#endif

    if (!options.verbosityChanged)
        options.verbosity = static_cast<Logger::Level>(
                               Configuration::getValue("log_gameServerLogLevel",
                                                       options.verbosity) );
    Logger::setVerbosity(options.verbosity);

    // When the gameListenToClientPort is set, we use it.
    // Otherwise, we use the accountListenToClientPort + 3 if the option is set.
    // If neither, the DEFAULT_SERVER_PORT + 3 is used.
    if (!options.portChanged)
    {
        // Prepare the fallback value
        options.port = Configuration::getValue("net_accountListenToClientPort",
                                               0) + 3;
        if (options.port == 3)
            options.port = DEFAULT_SERVER_PORT + 3;

        // Set the actual value of options.port
        options.port = Configuration::getValue("net_gameListenToClientPort",
                                               options.port);
    }

    // Make an initial attempt to connect to the account server
    // Try again after longer and longer intervals when connection fails.
    bool isConnected = false;
    int waittime = 0;
    while (!isConnected && running)
    {
        LOG_INFO("Connecting to account server");
        isConnected = accountHandler->start(options.port);
        if (!isConnected)
        {
            LOG_INFO("Retrying in " << ++waittime << " seconds");
            usleep(waittime * 1000);
        }
    }

    if (!gameHandler->startListen(options.port))
    {
        LOG_FATAL("Unable to create an ENet server host.");
        return EXIT_NET_EXCEPTION;
    }

    // Initialize world timer
    worldTimer.start();

    // Account connection lost flag
    bool accountServerLost = false;
    int elapsedWorldTicks = 0;

    while (running)
    {
        elapsedWorldTicks = worldTimer.poll();
        if (elapsedWorldTicks == 0) worldTimer.sleep();

        while (elapsedWorldTicks > 0)
        {
            if (elapsedWorldTicks > WORLD_TICK_SKIP)
            {
                LOG_WARN("Skipped "<< elapsedWorldTicks - 1
                        << " world tick due to insufficient CPU time.");
                elapsedWorldTicks = 1;
            }
            worldTime++;
            elapsedWorldTicks--;

            // Print world time at 10 second intervals to show we're alive
            if (worldTime % 100 == 0) {
                LOG_INFO("World time: " << worldTime);

            }

            if (accountHandler->isConnected())
            {
                accountServerLost = false;

                // Handle all messages that are in the message queues
                accountHandler->process();

                if (worldTime % 100 == 0) {
                    accountHandler->syncChanges(true);
                    // force sending changes to the account server every 10 secs.
                }

                if (worldTime % 300 == 0)
                {
                    accountHandler->sendStatistics();
                    LOG_INFO("Total Account Output: " << gBandwidth->totalInterServerOut() << " Bytes");
                    LOG_INFO("Total Account Input: " << gBandwidth->totalInterServerIn() << " Bytes");
                    LOG_INFO("Total Client Output: " << gBandwidth->totalClientOut() << " Bytes");
                    LOG_INFO("Total Client Input: " << gBandwidth->totalClientIn() << " Bytes");
                }
            }
            else
            {
                // If the connection to the account server is lost.
                // Every players have to be logged out
                if (!accountServerLost)
                {
                    LOG_WARN("The connection to the account server was lost.");
                    accountServerLost = true;
                }

                // Try to reconnect every 200 ticks
                if (worldTime % 200 == 0)
                {
                    accountHandler->start(options.port);
                }
            }
            gameHandler->process();
            // Update all active objects/beings
            GameState::update(worldTime);
            // Send potentially urgent outgoing messages
            gameHandler->flush();
        }
    }

    LOG_INFO("Received: Quit signal, closing down...");
    gameHandler->stopListen();
    accountHandler->stop();
    deinitializeServer();

    return EXIT_NORMAL;
}
