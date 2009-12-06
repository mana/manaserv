/*
 *  The Mana Server
 *  Copyright (C) 2008  The Mana World Development Team
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

#include "game-server/commandhandler.hpp"
#include "game-server/accountconnection.hpp"
#include "game-server/character.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/inventory.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/monster.hpp"
#include "game-server/monstermanager.hpp"
#include "game-server/state.hpp"

#include "common/transaction.hpp"

#include "utils/string.hpp"

static void say(const std::string error, Character *player)
{
    GameState::sayTo(player, NULL, error);
}

static bool checkPermission(Character *player, unsigned int permissions)
{
    if (player->getAccountLevel() & permissions)
    {
        return true;
    }

    say("Invalid permissions", player);

    return false;
}

static std::string getArgument(std::string &args)
{
    std::string argument = "";
    std::string::size_type pos = args.find(' ');
    if (pos != std::string::npos)
    {
        argument = args.substr(0, pos);
        args = args.substr(pos+1);
    }
    else
    {
        argument = args.substr(0);
        args = "";
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
    if (args == "")
    {
        if (player->getAccountLevel() & AL_PLAYER)
        {
            say("=General Commands=", player);
            say("@help [command]", player);
            say("@report <bug>", player);
            say("@where", player);
            say("@rights", player);
        }

        if (player->getAccountLevel() & AL_TESTER)
        {
            say("=Tester Commands=", player);
            say("@warp <character> <map> <x> <y>", player);
            say("@goto <character>", player);
        }

        if (player->getAccountLevel() & AL_GM)
        {
            say("=Game Master Commands=", player);
            say("@recall <character>", player);
            say("@ban <character> <length of time>", player);
        }

        if (player->getAccountLevel() & AL_DEV)
        {
            say("=Developer Commands=", player);
            say("@item <character> <item id> <amount>", player);
            say("@drop <item id> <amount>", player);
            say("@money <character> <amount>", player);
            say("@spawn <monster id> <number>", player);
            say("@attribute <character> <attribute> <value>", player);
        }

        if (player->getAccountLevel() & AL_ADMIN)
        {
            say("=Administrator Commands=", player);
            say("@reload", player);
            say("@setgroup <character> <AL level>", player);
            say("@announce <message>", player);
            say("@history <number of transactions>", player);
        }
    }
    else
    {

    }
}

static void handleWarp(Character *player, std::string &args)
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
    if (character == "" || mapstr == "" || xstr == "" || ystr == "")
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
            say("Invalid character, or they are offline", player);
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
    std::string msg = "User warped to " + xstr + "x" + ystr;
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_WARP, msg);
}

static void handleItem(Character *player, std::string &args)
{
    Character *other;
    ItemClass *ic;
    int value;
    int id;

    // get arguments
    std::string character = getArgument(args);
    std::string itemclass = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character == "" || itemclass == "" || valuestr == "")
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @item <character> <itemID> <amount>", player);
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

    // check we have a valid item
    if (!utils::isNumeric(itemclass))
    {
        say("Invalid item", player);
        return;
    }

    // put the itemclass id into an integer
    id = utils::stringToInt(itemclass);

    // check for valid item class
    ic = ItemManager::getItem(id);

    if (!ic)
    {
        say("Invalid item", player);
        return;
    }

    if (!utils::isNumeric(valuestr))
    {
        say("Invalid value", player);
        return;
    }

    value = utils::stringToInt(valuestr);

    if (value < 0)
    {
        say("Invalid amount", player);
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
    int value, id;

    // get arguments
    std::string itemclass = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (itemclass == "" || valuestr == "")
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @drop <itemID> <amount]>", player);
        return;
    }

    // check that itemclass id and value are really integers
    if (!utils::isNumeric(itemclass) || !utils::isNumeric(valuestr))
    {
        say("Invalid arguments passed.", player);
        return;
    }

    // put the item class id into an integer
    id = utils::stringToInt(itemclass);

    // check for valid item
    ic = ItemManager::getItem(id);
    if (!ic)
    {
        say("Invalid item", player);
        return;
    }

    // put the value into an integer
    value = utils::stringToInt(valuestr);

    if (value < 0)
    {
        say("Invalid amount", player);
        return;
    }

    // create the integer and put it on the map
    Item *item = new Item(ic, value);
    item->setMap(player->getMap());
    item->setPosition(player->getPosition());
    GameState::insertSafe(item);

    // log transaction
    std::stringstream str;
    str << "User created item " << ic->getDatabaseID();
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_DROP, str.str());
}

static void handleMoney(Character *player, std::string &args)
{
    Character *other;
    int value;

    // get arguments
    std::string character = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character == "" || valuestr == "")
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

static void handleSpawn(Character *player, std::string &args)
{
    MonsterClass *mc;
    MapComposite *map = player->getMap();
    const Point &pos = player->getPosition();
    int value, id;

    // get the arguments
    std::string monsterclass = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (monsterclass == "" || valuestr == "")
    {
        say("Invalid amount of arguments given.", player);
        say("Usage: @spawn <monsterID> <number>", player);
        return;
    }

    // check they are really numbers
    if (!utils::isNumeric(monsterclass) || !utils::isNumeric(valuestr))
    {
        say("Invalid arguments", player);
        return;
    }

    // put the monster class id into an integer
    id = utils::stringToInt(monsterclass);

    // check for valid monster
    mc = MonsterManager::getMonster(id);
    if (!mc)
    {
        say("Invalid monster", player);
        return;
    }

    // put the amount into an integer
    value = utils::stringToInt(valuestr);

    if (value < 0)
    {
        say("Invalid amount", player);
        return;
    }

    // create the monsters and put them on the map
    for (int i = 0; i < value; ++i)
    {
        Being *monster = new Monster(mc);
        monster->setMap(map);
        monster->setPosition(pos);
        monster->clearDestination();
        if (!GameState::insertSafe(monster))
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
    if (character == "")
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
}

static void handleRecall(Character *player, std::string &args)
{
    Character *other;

    // get the arguments
    std::string character = getArgument(args);

    // check all arguments are there
    if (character == "")
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

static void handleReload(Character *player)
{
    // reload the items and monsters
    ItemManager::reload();
    MonsterManager::reload();
}

static void handleBan(Character *player, std::string &args)
{
    Character *other;
    int length;

    // get arguments
    std::string character = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character == "" || valuestr == "")
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

    // check the length is really an integer
    if (!utils::isNumeric(valuestr))
    {
        say("Invalid argument", player);
        return;
    }

    // change the length to an integer
    length = utils::stringToInt(valuestr);

    if (length < 0)
    {
        say("Invalid length", player);
        return;
    }

    // ban the player
    accountHandler->banCharacter(other, length);

    // log transaction
    std::string msg = "User banned " + other->getName();
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_BAN, msg);
}

static void handleSetGroup(Character *player, std::string &args)
{
    Character *other;
    int level = 0;

    // get the arguments
    std::string character = getArgument(args);
    std::string levelstr = getArgument(args);

    // check all arguments are there
    if (character == "" || levelstr == "")
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @setgroup <character> <level>", player);
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

    // check which level they should be
    // refer to defines.h for level info
    if (levelstr == "AL_PLAYER")
    {
        level = AL_PLAYER;
    }
    else if (levelstr == "AL_TESTER")
    {
        level = AL_PLAYER | AL_TESTER;
    }
    else if (levelstr == "AL_GM")
    {
        level = AL_PLAYER | AL_TESTER | AL_GM;
    }
    else if (levelstr == "AL_DEV")
    {
        level = AL_PLAYER | AL_TESTER | AL_DEV;
    }
    else if (levelstr == "AL_ADMIN")
    {
        level = 255;
    }

    if (level == 0)
    {
        say("Invalid group", player);
        return;
    }

    // change the player's account level
    accountHandler->changeAccountLevel(other, level);

    // log transaction
    std::string msg = "User changed account level of " + other->getName() + " to " + levelstr;
    accountHandler->sendTransaction(player->getDatabaseID(), TRANS_CMD_SETGROUP, msg);
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
    if (character == "" || valuestr == "" || attrstr == "")
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
}

static void handleReport(Character *player, std::string &args)
{
    std::string bugReport = getArgument(args);

    if (bugReport == "")
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @report <message>", player);
        return;
    }

    // TODO: Send the report to a developer or something
}

static void handleAnnounce(Character *player, std::string &msg)
{
    if (msg == "")
    {
        say("Invalid number of arguments given.", player);
        say("Usage: @announce <message>", player);
        return;
    }

    GameState::sayToAll(msg);
}

static void handleWhere(Character *player)
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

static void handleRights(Character *player)
{
    std::stringstream str;
    str << "Your rights level is: "
        << player->getAccountLevel()
        << " (AL_PLAYER";
    if (player->getAccountLevel() &  AL_TESTER)
        str << ", AL_TESTER";
    if (player->getAccountLevel() &  AL_GM)
        str << ", AL_GM";
    if (player->getAccountLevel() &  AL_ADMIN)
        str << ", AL_ADMIN";
    str << ")";
    say(str.str(), player);
}

static void handleHistory(Character *player, std::string args)
{
    // TODO: Get args number of transactions and show them to the player
}

void CommandHandler::handleCommand(Character *player,
                                   const std::string &command)
{
    // get command type, and arguments
    // remove first character (the @)
    std::string::size_type pos = command.find(' ');
    std::string type(command, 1, pos == std::string::npos ? pos : pos - 1);
    std::string args(command, pos == std::string::npos ? command.size() : pos + 1);

    // handle the command
    if (type == "help")
    {
        if (checkPermission(player, AL_PLAYER))
            handleHelp(player, args);
    }
    else if (type == "where" || type == "location")
    {
        if (checkPermission(player, AL_PLAYER))
            handleWhere(player);
    }
    else if (type == "rights" || type == "right" || type == "permission")
    {
        if (checkPermission(player, AL_PLAYER))
            handleRights(player);
    }
    else if (type == "warp")
    {
        if (checkPermission(player, AL_TESTER))
            handleWarp(player, args);
    }
    else if (type == "item")
    {
        if (checkPermission(player, AL_DEV))
            handleItem(player, args);
    }
    else if (type == "drop")
    {
        if (checkPermission(player, AL_DEV))
            handleDrop(player, args);
    }
    else if (type == "money")
    {
        if (checkPermission(player, AL_DEV))
            handleMoney(player, args);
    }
    else if (type == "spawn")
    {
        if (checkPermission(player, AL_DEV))
            handleSpawn(player, args);
    }
    else if (type == "goto")
    {
        if (checkPermission(player, AL_TESTER))
            handleGoto(player, args);
    }
    else if (type == "recall")
    {
        if (checkPermission(player, AL_GM))
            handleRecall(player, args);
    }
    else if (type == "reload")
    {
        if (checkPermission(player, AL_ADMIN))
            handleReload(player);
    }
    else if (type == "ban")
    {
        if (checkPermission(player, AL_GM))
            handleBan(player, args);
    }
    else if (type == "setgroup")
    {
        if (checkPermission(player, AL_ADMIN))
            handleSetGroup(player, args);
    }
    else if (type == "attribute")
    {
        if (checkPermission(player, AL_DEV))
            handleAttribute(player, args);
    }
    else if (type == "report")
    {
        if (checkPermission(player, AL_PLAYER))
            handleReport(player, args);
    }
    else if (type == "announce")
    {
        if (checkPermission(player, AL_ADMIN))
            handleAnnounce(player, args);
    }
    else if (type == "history")
    {
        if (checkPermission(player, AL_ADMIN))
            handleHistory(player, args);
    }
    else
    {
        say("Command not found. Enter @help to view the list of available commands.", player);
    }
}
