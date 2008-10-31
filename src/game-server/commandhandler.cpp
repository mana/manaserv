/*
 *  The Mana World
 *  Copyright 2008 The Mana World Development Team
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

#include "commandhandler.hpp"
#include "accountconnection.hpp"
#include "character.hpp"
#include "gamehandler.hpp"
#include "inventory.hpp"
#include "item.hpp"
#include "itemmanager.hpp"
#include "mapmanager.hpp"
#include "monster.hpp"
#include "monstermanager.hpp"
#include "state.hpp"

#include "../utils/string.hpp"

#include <sstream>

void CommandHandler::handleCommand(Character *player,
                                   const std::string &command)
{
    // check character permissions
    // finer tuning for checking per command can be done
    // in the handle function for that command
    if (player->getAccountLevel() != AL_GM)
    {
        return;
    }

    // get command type, and arguments
    // remove first character (the @)
    std::string::size_type pos = command.find(' ');
    std::string type(command, 1, pos);
    std::string args(command, pos == std::string::npos ? command.size() : pos + 1);

    // handle the command
    if (type == "help")
    {
        handleHelp(player, args);
    }
    else if (type == "warp")
    {
        handleWarp(player, args);
    }
    else if (type == "item")
    {
        handleItem(player, args);
    }
    else if (type == "drop")
    {
        handleDrop(player, args);
    }
    else if (type == "money")
    {
        handleMoney(player, args);
    }
    else if (type == "spawn")
    {
        handleSpawn(player, args);
    }
    else if (type == "goto")
    {
        handleGoto(player, args);
    }
    else if (type == "recall")
    {
        handleRecall(player, args);
    }
    else if (type == "reload")
    {
        handleReload(player);
    }
    else if (type == "ban")
    {
        handleBan(player, args);
    }
    else if (type == "level")
    {
        handleLevel(player, args);
    }
    else if (type == "attribute")
    {
        handleAttribute(player, args);
    }
}

void CommandHandler::handleHelp(Character *player, std::string &args)
{
    if (args == "")
    {
        std::stringstream output;

        if (player->getAccountLevel() >= AL_GM)
        {
            // display GM commands
            output << "@help" << std::endl;
            output << "@warp" << std::endl;
            output << "@item" << std::endl;
            output << "@drop" << std::endl;
            output << "@money" << std::endl;
            output << "@spawn" << std::endl;
            output << "@goto" << std::endl;
            output << "@recall" << std::endl;
            output << "@ban" << std::endl;
            output << "@attribute" << std::endl;
        }

        if (player->getAccountLevel() == AL_ADMIN)
        {
            // display Admin commands
            output << "@reload" << std::endl;
            output << "@level" << std::endl;
        }

        GameState::sayTo(player, NULL, output.str());
    }
    else
    {

    }
}

void CommandHandler::handleWarp(Character *player, std::string &args)
{
    std::stringstream str;
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
        errorMsg("Invalid number of arguments given.", player);
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
            errorMsg("Invalid character, or they are offline", player);
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
            errorMsg("Invalid map", player);
            return;
        }

        str << mapstr;
        str >> id;
        str.str("");

        // get the map
        map = MapManager::getMap(id);
        if (!map)
        {
            errorMsg("Invalid map", player);
            return;
        }
    }

    if (!utils::isNumeric(xstr))
    {
        errorMsg("Invalid x", player);
        return;
    }

    if (!utils::isNumeric(ystr))
    {
        errorMsg("Invalid y", player);
        return;
    }

    // change the x and y to integers
    str << xstr;
    str >> x;
    str.str("");
    str << ystr;
    str >> y;
    str.str("");

    // now warp the player
    GameState::warp(other, map, x, y);
}

void CommandHandler::handleItem(Character *player, std::string &args)
{
    Character *other;
    ItemClass *ic;
    int value;
    int id;
    std::stringstream str;

    // get arguments
    std::string character = getArgument(args);
    std::string itemclass = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character == "" || itemclass == "" || valuestr == "")
    {
        errorMsg("Invalid number of arguments given.", player);
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
            errorMsg("Invalid character or they are offline", player);
            return;
        }
    }

    // check we have a valid item
    if (!utils::isNumeric(itemclass))
    {
        errorMsg("Invalid item", player);
        return;
    }

    // put the itemclass id into an integer
    str << itemclass;
    str >> id;
    str.str("");

    // check for valid item class
    ic = ItemManager::getItem(id);

    if (!ic)
    {
        errorMsg("Invalid item", player);
        return;
    }

    if (!utils::isNumeric(valuestr))
    {
        errorMsg("Invalid value", player);
        return;
    }

    str << valuestr;
    str >> value;
    str.str("");

    if (value < 0)
    {
        errorMsg("Invalid amount", player);
        return;
    }

    // insert the item into the inventory
    Inventory(other).insert(ic->getDatabaseID(), value);
}

void CommandHandler::handleDrop(Character *player, std::string &args)
{
    ItemClass *ic;
    int value, id;
    std::stringstream str;

    // get arguments
    std::string itemclass = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (itemclass == "" || valuestr == "")
    {
        errorMsg("Invalid number of arguments given.", player);
        return;
    }

    // check that itemclass id and value are really integers
    if (!utils::isNumeric(itemclass) || !utils::isNumeric(valuestr))
    {
        errorMsg("Invalid arguments passed.", player);
        return;
    }

    // put the item class id into an integer
    str << itemclass;
    str >> id;
    str.str("");

    // check for valid item
    ic = ItemManager::getItem(id);
    if (!ic)
    {
        errorMsg("Invalid item", player);
        return;
    }

    // put the value into an integer
    str << valuestr;
    str >> value;
    str.str("");

    if (value < 0)
    {
        errorMsg("Invalid amount", player);
        return;
    }

    // create the integer and put it on the map
    Item *item = new Item(ic, value);
    item->setMap(player->getMap());
    item->setPosition(player->getPosition());
    GameState::insertSafe(item);
}

void CommandHandler::handleMoney(Character *player, std::string &args)
{
    Character *other;
    int value;
    std::stringstream str;

    // get arguments
    std::string character = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character == "" || valuestr == "")
    {
        errorMsg("Invalid number of arguments given", player);
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
            errorMsg("Invalid character or they are offline", player);
            return;
        }
    }

    // check value is an integer
    if (!utils::isNumeric(valuestr))
    {
        errorMsg("Invalid argument", player);
        return;
    }

    // change value into an integer
    str << valuestr;
    str >> value;
    str.str("");

    // change how much money the player has
    Inventory(other).changeMoney(value);
}

void CommandHandler::handleSpawn(Character *player, std::string &args)
{
    MonsterClass *mc;
    MapComposite *map = player->getMap();
    Point const &pos = player->getPosition();
    int value, id;
    std::stringstream str;

    // get the arguments
    std::string monsterclass = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (monsterclass == "" || valuestr == "")
    {
        errorMsg("Invalid amount of arguments given.", player);
        return;
    }

    // check they are really numbers
    if (!utils::isNumeric(monsterclass) || !utils::isNumeric(valuestr))
    {
        errorMsg("Invalid arguments", player);
        return;
    }

    // put the monster class id into an integer
    str << monsterclass;
    str >> id;
    str.str("");

    // check for valid monster
    mc = MonsterManager::getMonster(id);
    if (!mc)
    {
        errorMsg("Invalid monster", player);
        return;
    }

    // put the amount into an integer
    str << valuestr;
    str >> value;
    str.str("");

    if (value < 0)
    {
        errorMsg("Invalid amount", player);
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
    }
}

void CommandHandler::handleGoto(Character *player, std::string &args)
{
    Character *other;

    // get the arguments
    std::string character = getArgument(args);

    // check all arguments are there
    if (character == "")
    {
        errorMsg("Invalid amount of arguments given.", player);
        return;
    }

    // check for valid player
    other = getPlayer(character);
    if (!other)
    {
        errorMsg("Invalid character, or they are offline.", player);
        return;
    }

    // move the player to where the other player is
    MapComposite *map = other->getMap();
    Point const &pos = other->getPosition();
    GameState::warp(player, map, pos.x, pos.y);
}

void CommandHandler::handleRecall(Character *player, std::string &args)
{
    Character *other;

    // get the arguments
    std::string character = getArgument(args);

    // check all arguments are there
    if (character == "")
    {
        errorMsg("Invalid amount of arguments given.", player);
        return;
    }

    // check for valid player
    other = getPlayer(character);
    if (!other)
    {
        errorMsg("Invalid character, or they are offline.", player);
        return;
    }

    // move the other player to where the player is
    MapComposite *map = player->getMap();
    Point const &pos = player->getPosition();
    GameState::warp(other, map, pos.x, pos.y);
}

void CommandHandler::handleReload(Character *player)
{
    // check for valid permissions
    // reload the items and monsters
    if (player->getAccountLevel() == AL_ADMIN)
    {
        ItemManager::reload();
        MonsterManager::reload();
    }
    else
    {
        errorMsg("Invalid permissions.", player);
    }
}

void CommandHandler::handleBan(Character *player, std::string &args)
{
    Character *other;
    int length;
    std::stringstream str;

    // get arguments
    std::string character = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character == "" || valuestr == "")
    {
        errorMsg("Invalid number of arguments given.", player);
        return;
    }

    // check for valid player
    other = getPlayer(character);
    if (!other)
    {
        errorMsg("Invalid character", player);
        return;
    }

    // check the length is really an integer
    if (!utils::isNumeric(valuestr))
    {
        errorMsg("Invalid argument", player);
        return;
    }

    // change the length to an integer
    str << valuestr;
    str >> length;
    str.str("");

    if (length < 0)
    {
        errorMsg("Invalid length", player);
        return;
    }

    // ban the player
    accountHandler->banCharacter(other, length);
}

void CommandHandler::handleLevel(Character *player, std::string &args)
{
    Character *other;
    int level;
    std::stringstream str;

    // get the arguments
    std::string character = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character == "" || valuestr == "")
    {
        errorMsg("Invalid number of arguments given.", player);
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
            errorMsg("Invalid character", player);
            return;
        }
    }

    // check the amount is really an integer
    if (!utils::isNumeric(valuestr))
    {
        errorMsg("Invalid argument", player);
        return;
    }

    // put the amount into an integer
    str << valuestr;
    str >> level;
    str.str("");

    if (level < 0)
    {
        errorMsg("Invalid level", player);
        return;
    }

    // change the player's account level
    accountHandler->changeAccountLevel(other, level);
}

void CommandHandler::handleAttribute(Character *player, std::string &args)
{
    Character *other;
    int attr, value;
    std::stringstream str;

    // get arguments
    std::string character = getArgument(args);
    std::string attrstr = getArgument(args);
    std::string valuestr = getArgument(args);

    // check all arguments are there
    if (character == "" || valuestr == "" || attrstr == "")
    {
        errorMsg("Invalid number of arguments given.", player);
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
            errorMsg("Invalid character", player);
            return;
        }
    }

    // check they are really integers
    if (!utils::isNumeric(valuestr) || !utils::isNumeric(attrstr))
    {
        errorMsg("Invalid argument", player);
        return;
    }

    // put the attribute into an integer
    str << attrstr;
    str >> attr;
    str.str("");

    if (attr < 0)
    {
        errorMsg("Invalid Attribute", player);
        return;
    }

    // put the value into an integer
    str << valuestr;
    str >> value;
    str.str("");

    if (value < 0)
    {
        errorMsg("Invalid amount", player);
        return;
    }

    // change the player's attribute
    other->setAttribute(attr, value);
}

void CommandHandler::errorMsg(const std::string error, Character *player)
{
    // output an error
    GameState::sayTo(player, NULL, error);
}

std::string CommandHandler::getArgument(std::string &args)
{
    std::string argument = "";
    std::string::size_type pos = args.find(' ');
    if (pos != std::string::npos)
    {
        argument = args.substr(0, pos-1);
        args = args.substr(pos+1);
    }

    return argument;
}

Character* CommandHandler::getPlayer(const std::string &player)
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
