/*
 *  The Mana Server
 *  Copyright (C) 2008-2010  The Mana World Development Team
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

#include <sstream>

#include "game-server/commandhandler.h"
#include "game-server/accountconnection.h"
#include "game-server/character.h"
#include "game-server/gamehandler.h"
#include "game-server/inventory.h"
#include "game-server/item.h"
#include "game-server/itemmanager.h"
#include "game-server/mapcomposite.h"
#include "game-server/mapmanager.h"
#include "game-server/monster.h"
#include "game-server/monstermanager.h"
#include "game-server/state.h"

#include "scripting/script.h"

#include "common/configuration.h"
#include "common/permissionmanager.h"
#include "common/transaction.h"

#include "utils/string.h"

struct CmdRef
{
    const char *cmd;
    const char *usage;
    const char *help;
    void (*func)(Character*, std::string&) ;
};

static void handleHelp(Character*, std::string&);
static void handleReport(Character*, std::string&);
static void handleWhere(Character*, std::string&);
static void handleRights(Character*, std::string&);
static void handleWarp(Character*, std::string&);
static void handleCharWarp(Character*, std::string&);
static void handleGoto(Character*, std::string&);
static void handleRecall(Character*, std::string&);
static void handleBan(Character*, std::string&);
static void handleItem(Character*, std::string&);
static void handleDrop(Character*, std::string&);
//static void handleMoney(Character*, std::string&);
static void handleSpawn(Character*, std::string&);
static void handleAttribute(Character*, std::string&);
static void handleReload(Character*, std::string&);
static void handleGivePermission(Character*, std::string&);
static void handleTakePermission(Character*, std::string&);
static void handleAnnounce(Character*, std::string&);
static void handleHistory(Character*, std::string&);
static void handleMute(Character*, std::string&);
static void handleDie(Character*, std::string&);
static void handleKill(Character*, std::string&);
static void handleKick(Character*, std::string&);
static void handleLog(Character*, std::string&);
static void handleLogsay(Character*, std::string&);
static void handleKillMonsters(Character*, std::string&);
static void handleCraft(Character*, std::string&);

static CmdRef const cmdRef[] =
{
    {"help",   "[command]" ,
        "Lists all available commands or a detailed help for a command", &handleHelp },
    {"report", "<bug>"     ,
        "Sends a bug or abuse reports a bug to the server administration", &handleReport},
    {"where",  ""          ,
        "Tells you your location in the game world", &handleWhere},
    {"rights", ""          ,
        "Tells you your current permissions", &handleRights},
    {"warp",   "<map> <x> <y>",
        "Teleports your character to a different location in the game world", &handleWarp},
    {"charwarp",   "<character> <map> <x> <y>",
        "Teleports the given character to a different location in the game world", &handleCharWarp},
    {"goto", "<character>",
        "Teleports you to the location of another character", &handleGoto},
    {"recall", "<character>",
        "Teleports another character to your location", &handleRecall},
    {"ban", "<character> <length of time>(m|h|d|w|y)",
        "Bans the character and all characters on the same account from the game", &handleBan},
    {"item", "<character> <item id> <amount>",
        "Creates a number of items in the inventory of a character", &handleItem},
    {"drop", "<item id> <amount>",
        "Drops a stack of items on the ground at your current location", &handleDrop},
/*    {"money", "<character> <amount>",
        "Changes the money a character possesses", &handleMoney},*/
    {"spawn", "<monster id> <number>",
        "Creates a number of monsters near your location", &handleSpawn},
    {"attribute", "<character> <attribute> <value>",
        "Changes the character attributes of a character", &handleAttribute},
    {"reload", "",
        "Makes the server reload all configuration files", &handleReload},
    {"givepermission", "<character> <permission class>",
        "Gives a permission class to the account a character belongs to", &handleGivePermission},
    {"takepermission", "<character> <permission class>",
        "Takes a permission class from the account a character belongs to", &handleTakePermission},
    {"announce", "<message>",
        "Sends a chat message to all characters in the game", &handleAnnounce},
    {"history", "<number of transactions>",
        "Shows the last transactions", &handleHistory},
    {"mute","<character> <length in seconds>",
        "Prevents the character from talking for the specified number of seconds. Use 0 seconds to unmute.", &handleMute},
    {"die", "",
        "Kills you.", &handleDie},
    {"kill", "<character>",
        "Kills the character.", &handleKill},
    {"kick", "<character>",
        "Disconnects the client of character.", &handleKick},
    {"log", "<message>",
        "Logs a message to the transaction log.", &handleLog},
    {"logsay", "<message>",
        "Says something in public chat while logging it to the transaction log.", &handleLogsay},
    {"killmonsters", "",
        "Kills all monsters on the map.", &handleKillMonsters},
    {"craft", "{ <item> <amount> }",
        "Crafts something.", &handleCraft},
    {NULL, NULL, NULL, NULL}

};

static void say(const std::string error, Character *player)
{
    GameState::sayTo(player, NULL, error);
}

/*
static bool checkPermission(Character *player, unsigned int permissions)
{
    if (player->getAccountLevel() & permissions)
    {
        return true;
    }

    say("Invalid permissions", player);

    return false;
}*/

/**
 * Returns the next argument, and remove it from the given string.
 */
static std::string getArgument(std::string &args)
{
    std::string argument;
    std::string::size_type pos = std::string::npos;
    bool doubleQuotes = false;

    // Finds out if the next argument is between double-quotes
    if (args.empty() || args.at(0) != '"')
    {
        // No double-quotes, we then search an ending space.
        pos = args.find(' ');
        doubleQuotes = false;
    }
    else
    {
        // Exclude the first double-quote from search.
        pos = args.find('"', 1);
        doubleQuotes = true;
    }

    if (pos != std::string::npos)
    {
        argument = args.substr(0, pos);
        if (doubleQuotes)
        {
            // Jumps to the next parameter,
            // after the ending double-quote and space,
            // and remove the two double-quotes before returning.
            args = args.substr(pos + 2);
            argument = argument.substr(1, pos - 1);
        }
        else
        {
            // Jumps to the next parameter, after the ending space.
            args = args.substr(pos + 1);
        }
    }
    else
    {
        argument = args.substr(0);
        args = std::string();
    }
    return argument;
}

static Character* getPlayer(const std::string &player)
{
    // get character, via the client, as they may be
    // on a different game server
    GameClient *client = gameHandler->getClientByNameSlow(player);
    if (!client)
    {
        return NULL;
    }

    if (client->status != CLIENT_CONNECTED)
    {
        return NULL;
    }

    return client->character;
}

static void handleHelp(Character *player, std::string &args)
{
    if (args.empty())
    {
        // short list of all commands
        say("=Available Commands=", player);
        std::list<std::string> commands = PermissionManager::getPermissionList(player);
        for (std::list<std::string>::iterator i = commands.begin();
             i != commands.end();
             i++)
        {
            say((*i), player);
        }
    } else {
        // don't show help for commands the player may not use
        if (PermissionManager::checkPermission(player, "@"+args) == PermissionManager::PMR_DENIED)
        {
            say("Why do you want to know? You can't use it anyway!", player);
            return;
        }
        // detailed description of single command
        for (size_t j = 0; cmdRef[j].cmd != NULL; j++)
        {
            if (cmdRef[j].cmd == args)
            {
                std::string msg;
                msg.append("@");
                msg.append(cmdRef[j].cmd);
                msg.append(" ");
                msg.append(cmdRef[j].usage);
                say(msg, player);
                say(cmdRef[j].help, player);
                return;
            }
        }
        say("There is no command @"+args, player);
    }
}

static void handleWarp(Character *player, std::string &args)
{
    int x, y;
    MapComposite *map;

    // get the arguments
    std::string mapstr = getArgument(args);
    std::string xstr = getArgument(args);
    std::string ystr = getArgument(args);

    // if any of them are empty strings, no argument was given
    if (mapstr.empty() || xstr.empty() || ystr.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @warp <map> <x> <y>", player);
        return;
    }

    // if it contains # then it means the player's map
    if (mapstr == "#")
    {
        map = player->getMap();
    }
    else
    {
        if (mapstr[0] == '#')
        {
            mapstr = mapstr.substr(1);
            // check for valid map id
            int id;
            if (!utils::isNumeric(mapstr))
            {
                say("Invalid map", player);
                return;
            }

            id = utils::stringToInt(mapstr);

            // get the map
            map = MapManager::getMap(id);
            if (!map)
            {
                say("Invalid map", player);
                return;
            }
        }
        else
        {
            map = MapManager::getMap(mapstr);

            if (!map)
            {
                say("Invalid map", player);
                return;
            }
        }
    }

    if (!utils::isNumeric(xstr))
    {
        say("Invalid x", player);
        return;
    }

    if (!utils::isNumeric(ystr))
    {
        say("Invalid y", player);
        return;
    }

    // change the x and y to integers
    x = utils::stringToInt(xstr);
    y = utils::stringToInt(ystr);

    // now warp the player
    GameState::warp(player, map, x, y);

    // log transaction
    std::stringstream ss;
    ss << "User warped to " << map->getName() << " (" << x << ", " << y << ")";
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_WARP,
                                    ss.str());
}

static void handleCharWarp(Character *player, std::string &args)
{
    int x, y;
    MapComposite *map;
    Character *other;

    // get the arguments
    std::string character = getArgument(args);
    std::string mapstr = getArgument(args);
    std::string xstr = getArgument(args);
    std::string ystr = getArgument(args);

    // if any of them are empty strings, no argument was given
    if (character.empty() || mapstr.empty() || xstr.empty() || ystr.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @warp <character> <map> <x> <y>", player);
        return;
    }

    // if it contains # then it means the player
    if (character == "#")
    {
        other = player;
    }
    else
    {
        // check for valid player
        other = getPlayer(character);
        if (!other)
        {
            say("Invalid or offline character <" + character + ">.", player);
            return;
        }
    }

    // if it contains # then it means the player's map
    if (mapstr == "#")
    {
        map = player->getMap();
    }
    else
    {
        if (mapstr[0] == '#')
        {
            mapstr = mapstr.substr(1);
            // check for valid map id
            int id;
            if (!utils::isNumeric(mapstr))
            {
                say("Invalid map", player);
                return;
            }

            id = utils::stringToInt(mapstr);

            // get the map
            map = MapManager::getMap(id);
            if (!map)
            {
                say("Invalid map", player);
                return;
            }
        }
        else
        {
            map = MapManager::getMap(mapstr);

            if (!map)
            {
                say("Invalid map", player);
                return;
            }
        }
    }

    if (!utils::isNumeric(xstr))
    {
        say("Invalid x", player);
        return;
    }

    if (!utils::isNumeric(ystr))
    {
        say("Invalid y", player);
        return;
    }

    // change the x and y to integers
    x = utils::stringToInt(xstr);
    y = utils::stringToInt(ystr);

    // now warp the player
    GameState::warp(other, map, x, y);

    // log transaction
    std::stringstream ss;
    ss << "User warped " << other->getName() << " to " << map->getName() <<
            " (" << x << ", " << y << ")";
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_WARP,
                                    ss.str());
}

static void handleItem(Character *player, std::string &args)
{
    Character *other;
    ItemClass *ic;
    int value = 0;

    // get arguments
    std::string character = getArgument(args);
    std::string itemclass = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character.empty() || itemclass.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @item <character> <item> [amount]", player);
        return;
    }

    // if it contains # that means the player
    if (character == "#")
    {
        other = player;
    }
    else
    {
        // check for valid player
        other = getPlayer(character);
        if (!other)
        {
            say("Invalid character or they are offline", player);
            return;
        }
    }

    // identify the item type
    if (utils::isNumeric(itemclass))
    {
        int id = utils::stringToInt(itemclass);
        ic = itemManager->getItem(id);
    }
    else
    {
        ic = itemManager->getItemByName(itemclass);
    }
    if (!ic)
    {
        say("Invalid item", player);
        return;
    }

    //identify the amount
    if  (valuestr.empty())
    {
        value = 1;
    }
    else if (utils::isNumeric(valuestr))
    {
        value = utils::stringToInt(valuestr);
    }
    // check for valid amount
    if (value <= 0)
    {
        say("Invalid number of items", player);
        return;
    }

    // insert the item into the inventory
    Inventory(other).insert(ic->getDatabaseID(), value);

    // log transaction
    std::stringstream str;
    str << "User created item " << ic->getDatabaseID();
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_ITEM, str.str());
}

static void handleDrop(Character *player, std::string &args)
{
    ItemClass *ic;
    int value = 0;

    // get arguments
    std::string itemclass = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (itemclass.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @drop <item> [amount]", player);
        return;
    }

    // identify the item type
    if (utils::isNumeric(itemclass))
    {
        int id = utils::stringToInt(itemclass);
        ic = itemManager->getItem(id);
    }
    else
    {
        ic = itemManager->getItemByName(itemclass);
    }
    if (!ic)
    {
        say("Invalid item", player);
        return;
    }

    //identify the amount
    if  (valuestr.empty())
    {
        value = 1;
    }
    else if (utils::isNumeric(valuestr))
    {
        value = utils::stringToInt(valuestr);
    }
    // check for valid amount
    if (value <= 0)
    {
        say("Invalid number of items", player);
        return;
    }

    // create the integer and put it on the map
    Item *item = new Item(ic, value);
    item->setMap(player->getMap());
    item->setPosition(player->getPosition());
    GameState::insertOrDelete(item);

    // log transaction
    std::stringstream str;
    str << "User created item " << ic->getDatabaseID();
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_DROP, str.str());
}
/*
static void handleMoney(Character *player, std::string &args)
{
    Character *other;
    int value;

    // get arguments
    std::string character = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character.empty() || valuestr.empty())
    {
        say("Invalid number of arguments given", player);
        say("Usage: @money <character> <amount>", player);
        return;
    }

    // check if its the player itself
    if (character == "#")
    {
        other = player;
    }
    else
    {
        // check for valid player
        other = getPlayer(character);
        if (!other)
        {
            say("Invalid character or they are offline", player);
            return;
        }
    }

    // check value is an integer
    if (!utils::isNumeric(valuestr))
    {
        say("Invalid argument", player);
        return;
    }

    // change value into an integer
    value = utils::stringToInt(valuestr);

    // change how much money the player has
    Inventory(other).changeMoney(value);

    // log transaction
    std::string msg = "User created " + valuestr + " money";
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_MONEY, msg);
}
*/

static void handleSpawn(Character *player, std::string &args)
{
    MonsterClass *mc;
    MapComposite *map = player->getMap();
    const Point &pos = player->getPosition();
    int value = 0;

    // get the arguments
    std::string monsterclass = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (monsterclass.empty())
    {
        say("Invalid amount of arguments given.", player);
        say("Usage: @spawn <monster> [number]", player);
        return;
    }

    // identify the monster type
    if (utils::isNumeric(monsterclass))
    {
        int id = utils::stringToInt(monsterclass);
        mc = monsterManager->getMonster(id);
    }
    else
    {
        mc = monsterManager->getMonsterByName(monsterclass);
    }
    // check for valid monster
    if (!mc)
    {
        say("Invalid monster", player);
        return;
    }

    //identify the amount
    if  (valuestr.empty())
    {
        value = 1;
    }
    else if (utils::isNumeric(valuestr))
    {
        value = utils::stringToInt(valuestr);
    }
    // check for valid amount
    if (value <= 0)
    {
        say("Invalid number of monsters", player);
        return;
    }

    // create the monsters and put them on the map
    for (int i = 0; i < value; ++i)
    {
        Being *monster = new Monster(mc);
        monster->setMap(map);
        monster->setPosition(pos);
        monster->clearDestination();
        if (!GameState::insertOrDelete(monster))
        {
            // The map is full. Break out.
            break;
        }

        // log transaction
        std::string msg = "User created monster " + monster->getName();
        accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_SPAWN, msg);
    }
}

static void handleGoto(Character *player, std::string &args)
{
    Character *other;

    // get the arguments
    std::string character = getArgument(args);

    // check all arguments are there
    if (character.empty())
    {
        say("Invalid amount of arguments given.", player);
        say("Usage: @goto <character>", player);
        return;
    }

    // check for valid player
    other = getPlayer(character);
    if (!other)
    {
        say("Invalid character, or they are offline.", player);
        return;
    }

    // move the player to where the other player is
    MapComposite *map = other->getMap();
    const Point &pos = other->getPosition();
    GameState::warp(player, map, pos.x, pos.y);

    // log transaction
    std::stringstream msg;
    msg << "User warped own character to " << other->getName();
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_GOTO, msg.str());
}

static void handleRecall(Character *player, std::string &args)
{
    Character *other;

    // get the arguments
    std::string character = getArgument(args);

    // check all arguments are there
    if (character.empty())
    {
        say("Invalid amount of arguments given.", player);
        say("Usage: @recall <character>", player);
        return;
    }

    // check for valid player
    other = getPlayer(character);
    if (!other)
    {
        say("Invalid character, or they are offline.", player);
        return;
    }

    // move the other player to where the player is
    MapComposite *map = player->getMap();
    const Point &pos = player->getPosition();
    GameState::warp(other, map, pos.x, pos.y);
}

static void handleReload(Character *, std::string &)
{
    // reload the items and monsters
    itemManager->reload();
    monsterManager->reload();
}

static void handleBan(Character *player, std::string &args)
{
    Character *other;
    int length;
    int lengthMutiplier = 0;

    // get arguments
    std::string character = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character.empty() || valuestr.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @ban <character> <duration>", player);
        return;
    }

    // check for valid player
    other = getPlayer(character);
    if (!other)
    {
        say("Invalid character", player);
        return;
    }

    // get the unit
    char unit = valuestr.at(valuestr.length()-1);
    switch (unit)
    {
        case 'm':
            lengthMutiplier = 1;
            break;
        case 'h':
            lengthMutiplier = 60;
            break;
        case 'd':
            lengthMutiplier = 60 * 24;
            break;
        case 'w':
            lengthMutiplier = 60 * 24 * 7;
            break;
        case 'y':
            lengthMutiplier = 60 * 24 * 365;
            break;
    }
    length = utils::stringToInt(valuestr.substr(0, valuestr.length()-1));
    length = length * lengthMutiplier;
    if (length <= 0)
    {
        std::string errmsg;
        errmsg += "Invalid length. Please enter a positive number ";
        errmsg += "followed by the letter m, h, d, w or y for minutes ";
        errmsg += ", hours, days, weeks or years.";
        say(errmsg , player);
        return;
    }

    // ban the player
    accountHandler->banCharacter(other, length);
    // disconnect the player
    MessageOut kickmsg(GPMSG_CONNECT_RESPONSE);
    kickmsg.writeInt8(ERRMSG_ADMINISTRATIVE_LOGOFF);
    other->getClient()->disconnect(kickmsg);

    // feedback for command user
    std::string msg = "You've banned " + other->getName() + " for " + utils::toString(length) + " minutes";
    say(msg.c_str(), player);
    // log transaction
    msg = "User banned " + other->getName() + " for " + utils::toString(length) + " minutes";
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_BAN, msg);
}

static void handleGivePermission(Character *player, std::string &args)
{
    Character *other;

    // get the arguments
    std::string character = getArgument(args);
    std::string strPermission = getArgument(args);

    // check all arguments are there
    if (character.empty() || strPermission.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @givepermission <character> <permission class>", player);
        return;
    }

    // check if its to effect the player
    if (character == "#")
    {
        other = player;
    }
    else
    {
        // check for valid player
        other = getPlayer(character);
        if (!other)
        {
            say("Invalid character", player);
            return;
        }
    }

    unsigned char permission = PermissionManager::getMaskFromAlias(strPermission);

    if (permission == 0x00)
    {
        say ("Unknown permission class: "+strPermission, player);
        return;
    }

    if (permission & other->getAccountLevel())
    {
        say(player->getName()+" already has the permission "+strPermission, player);
    }
    else
    {
        permission += other->getAccountLevel();
        // change the player's account level
        other->setAccountLevel(permission);
        accountHandler->changeAccountLevel(other, permission);

        // log transaction
        std::string msg = "User gave right " + strPermission + " to " + other->getName();
        accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_SETGROUP, msg);
        say("Congratulations, "+player->getName()+" gave you the rights of a "+strPermission, other);
    }
}

static void handleTakePermission(Character *player, std::string &args)
{
    Character *other;

    // get the arguments
    std::string character = getArgument(args);
    std::string strPermission = getArgument(args);

    // check all arguments are there
    if (character.empty() || strPermission.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @takepermission <character> <permission class>", player);
        return;
    }

    // check if its to effect the player
    if (character == "#")
    {
        other = player;
    }
    else
    {
        // check for valid player
        other = getPlayer(character);
        if (!other)
        {
            say("Invalid character", player);
            return;
        }
    }

    unsigned char permission = PermissionManager::getMaskFromAlias(strPermission);

    if (permission == 0x00)
    {
        say("Unknown permission class: "+strPermission, player);
        return;
    }


    if (!(permission & other->getAccountLevel()))
    {
        say(player->getName()+" hasn't got the permission "+strPermission, player);
    } else {
        permission = other->getAccountLevel() - permission;
        // change the player's account level
        other->setAccountLevel(permission);
        accountHandler->changeAccountLevel(other, permission);

        // log transaction
        std::string msg = "User took right " + strPermission + " from " + other->getName();
        accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_SETGROUP, msg);
        say("Sorry, "+player->getName()+" revoked your rights of a "+strPermission, other);
    }
}


static void handleAttribute(Character *player, std::string &args)
{
    Character *other;
    int attr, value;

    // get arguments
    std::string character = getArgument(args);
    std::string attrstr = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character.empty() || valuestr.empty() || attrstr.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @attribute <character> <attribute> <value>", player);
        return;
    }

    // check if its the player or another player
    if (character == "#")
    {
        other = player;
    }
    else
    {
        // check for valid player
        other = getPlayer(character);
        if (!other)
        {
            say("Invalid character", player);
            return;
        }
    }

    // check they are really integers
    if (!utils::isNumeric(valuestr) || !utils::isNumeric(attrstr))
    {
        say("Invalid argument", player);
        return;
    }

    // put the attribute into an integer
    attr = utils::stringToInt(attrstr);

    if (attr < 0)
    {
        say("Invalid Attribute", player);
        return;
    }

    // put the value into an integer
    value = utils::stringToInt(valuestr);

    if (value < 0)
    {
        say("Invalid amount", player);
        return;
    }

    // change the player's attribute
    other->setAttribute(attr, value);

    // log transaction
    std::stringstream msg;
    msg << "User changed attribute " << attr << " of player " << other->getName()
        << " to " << value;
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_ATTRIBUTE, msg.str());
}

static void handleReport(Character *player, std::string &args)
{
    std::string bugReport = getArgument(args);

    if (bugReport.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @report <message>", player);
        return;
    }

    // TODO: Send the report to a developer or something
}

static void handleAnnounce(Character *player, std::string &msg)
{
    if (msg.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @announce <message>", player);
        return;
    }

    GameState::sayToAll(msg);

    // log transaction
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_ANNOUNCE, msg);
}

static void handleWhere(Character *player, std::string &)
{
    std::stringstream str;
    str << "Your current location is map "
        << player->getMapId()
        << " ["
        << player->getPosition().x
        << ":"
        << player->getPosition().y
        << "]";
    say (str.str(), player);
}

static void handleRights(Character *player, std::string &)
{
    std::list<std::string>classes;
    classes = PermissionManager::getClassList(player);

    std::stringstream str;
    str << "Your rights level is: "
        << (unsigned int)player->getAccountLevel()
        << " ( ";

    for (std::list<std::string>::iterator i = classes.begin();
         i != classes.end();
         i++)
    {
        str << (*i) << " ";
    }
    str << ")";

    say(str.str(), player);
}

static void handleHistory(Character *player, std::string &args)
{
    // TODO: Get args number of transactions and show them to the player
}

static void handleMute(Character *player, std::string &args)
{
    Character *other;
    int length;

    // Get arguments.
    std::string character = getArgument(args);
    std::string valuestr = getArgument(args);


    // Check for a valid player.
    other = getPlayer(character);
    if (!other)
    {
        say("Invalid character", player);
        return;
    }

    // Turn the length back to an integer.
    if (valuestr.empty())
        length = Configuration::getValue("command_defaultMuteLength", 60);
    else
        length = utils::stringToInt(valuestr);

    if (length < 0)
    {
        say("Invalid length, using default", player);
        length = Configuration::getValue("command_defaultMuteLength", 60);
    }

    // Mute the player.
    other->mute(length);

    // Feedback.
    std::stringstream targetMsg;
    std::stringstream userMsg;
    if (length > 0)
    {
        targetMsg << player->getName() << " muted you for "
        << length << " seconds.";

        userMsg << "You muted " << other->getName()
        << " for " << length << " seconds.";
    }
    else
    {
        targetMsg << player->getName() << " unmuted you.";
        userMsg << "You unmuted " << other->getName() << ".";
    }
    say(targetMsg.str(), other);
    say(userMsg.str(), player);

    // log transaction
    std::stringstream msg;
    if (length > 0)
    {
        msg << "User muted " << other->getName() << " for " << length << " seconds.";
    } else {
        msg << "User unmuted " << other->getName();
    }
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_MUTE, msg.str());
}

static void handleDie(Character *player, std::string &args)
{
    player->setAttribute(ATTR_HP, 0);
    say("You've killed yourself.", player);
}

static void handleKill(Character *player, std::string &args)
{
    Character *other;

    // get arguments
    std::string character = getArgument(args);

    // check for valid player
    other = getPlayer(character);
    if (!other)
    {
        say("Invalid character", player);
        return;
    }

    // kill the player
    player->setAttribute(ATTR_HP, 0);

    // feedback
    std::stringstream targetMsg;
    std::stringstream userMsg;
    targetMsg << "You were killed by server command from "<< player->getName() << ".";
    userMsg << "You killed " << other->getName() << ".";
    say(targetMsg.str(), other);
    say(userMsg.str(), player);

    // log transaction
    std::stringstream logMsg;
    logMsg << "User killed " << other->getName();
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_KILL, logMsg.str());
}

static void handleKick(Character *player, std::string &args)
{
    Character *other;

    // get arguments
    std::string character = getArgument(args);

    // check for valid player
    other = getPlayer(character);
    if (!other)
    {
        say("Invalid character", player);
        return;
    }

    // send feedback
    std::stringstream userMsg;
    userMsg << "You kicked " << other->getName() << ".";
    say(userMsg.str(), player);

    // disconnect the client
    MessageOut msg(GPMSG_CONNECT_RESPONSE);
    msg.writeInt8(ERRMSG_ADMINISTRATIVE_LOGOFF);
    other->getClient()->disconnect(msg);

    // log transaction
    std::stringstream logMsg;
    logMsg << "User kicked " << other->getName();
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_KICK, logMsg.str());
}


static void handleLog(Character *player, std::string &msg)
{
    if (msg.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @log <message>", player);
        return;
    }

    // log transaction
    std::string logmsg = "[silent] " + msg;
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_LOG, logmsg);

    // send feedback
    say("Message logged", player);
}

static void handleLogsay(Character *player, std::string &msg)
{
    if (msg.empty())
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @logsay <message>", player);
        return;
    }

    GameState::sayAround(player, msg);

    // log transaction
    std::string logmsg = "[public] " + msg;
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_LOG, logmsg);

    // send feedback
    say("Message logged", player);
}

static void handleKillMonsters(Character *player, std::string &args)
{
    const MapComposite *map = player->getMap();
    int count = 0;

    for (BeingIterator it(map->getWholeMapIterator()); it; ++it)
    {
        if ((*it)->getType() == OBJECT_MONSTER && (*it)->getAction() != DEAD)
        {
            (*it)->died();
            count++;
        }
    }

    std::stringstream ss;
    ss << "You killed " << count << " monster" << (count > 1 ? "s." : ".");
    say(ss.str(), player);

    // log transaction
    std::string msg = "User killed all monsters on map " + map->getName();
    accountHandler->sendTransaction(player->getDatabaseID(),
                                    TRANS_CMD_KILLMONSTERS, msg);
}

static void handleCraft(Character *player, std::string &args)
{
    std::stringstream errMsg;
    std::list<InventoryItem> recipe;
    Inventory playerInventory(player);
    std::map<int, int> totalAmountOfItem;

    while (true)
    {
        // parsing
        std::string strItem = getArgument(args);
        ItemClass* item = itemManager->getItemByName(strItem);
        std::string strAmount = getArgument(args);
        int amount = utils::stringToInt(strAmount);

        // syntax error checking
        if (strItem.empty())
        {
            // the item list has ended
            break;
        }
        if (!item)
        {
            // item wasn't found in the item database
            errMsg << "Unknown item: \"" << strItem << "\".";
            break;
        }

        if (strAmount.empty())
        {
            // the last item in the list has no amount defined
            errMsg << "No amount given for \"" << strItem << "\".";
            break;
        }
        if (amount < 1)
        {
            errMsg << "Illegal amount \""<< strAmount << "\" for item \"" << strItem << "\".";
            break;
        }

        // inventory checking
        int available = playerInventory.count(item->getDatabaseID());
        if (available == 0)
        {
            errMsg << "You have no "<< strItem << " in your inventory.";
            break;
        }
        if (available < amount)
        {
            errMsg << "You haven't got that many "<< strItem << "s in your inventory.";
            break;
        }

        // when there is still no break, add the item;
        InventoryItem recipeItem;
        recipeItem.itemId = item->getDatabaseID();
        recipeItem.amount = amount;
        recipe.push_back(recipeItem);
    }

    if (!errMsg.str().empty())
    {
        // when an error occured, output the error
        say(errMsg.str(), player);
        return;
    } else {
        // pass to script engine. The engine is responsible for all
        // further processing of the crafting operation, including
        // outputting an error message when the recipe is invalid.
        Script::performCraft(player, recipe);
    }
}

void CommandHandler::handleCommand(Character *player,
                                   const std::string &command)
{
    // get command type, and arguments
    // remove first character (the @)
    std::string::size_type pos = command.find(' ');
    std::string type(command, 1, pos == std::string::npos ? pos : pos - 1);
    std::string args(command, pos == std::string::npos ? command.size() : pos + 1);

    PermissionManager::Result r = PermissionManager::checkPermission(player, "@"+type);
    switch (r)
    {
    case PermissionManager::PMR_DENIED:
        say("Permissions denied!", player);
        break;
    case PermissionManager::PMR_UNKNOWN:
        say("Unknown command. Enter @help to view the list of available commands.", player);
        break;
    case PermissionManager::PMR_ALLOWED:
        // handle the command
        for (size_t i = 0; cmdRef[i].cmd != NULL; i++)
        {
            if (cmdRef[i].cmd == type)
            {
                cmdRef[i].func(player,args);
                return;
            };
        }

        say("Command not implemented.", player);
        break;
    }
}
