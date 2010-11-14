/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
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

#include <cstddef>

#include "game-server/accountconnection.h"
#include "game-server/character.h"
#include "game-server/gamehandler.h"
#include "game-server/inventory.h"
#include "game-server/item.h"
#include "game-server/itemmanager.h"
#include "game-server/mapmanager.h"
#include "game-server/monster.h"
#include "game-server/monstermanager.h"
#include "game-server/state.h"

template< typename T >
static T proxy_cast(intptr_t v)
{ return (T)v; }

template<>
const std::string &proxy_cast(intptr_t v)
{ return *(const std::string *)v; }

template< typename T1 >
static void proxy(void (*f)(), Character *from, intptr_t const args[1])
{
    ((void (*)(Character *, T1))f)
        (from, proxy_cast<T1>(args[0]));
}

template< typename T1, typename T2 >
static void proxy(void (*f)(), Character *from, intptr_t const args[2])
{
    ((void (*)(Character *, T1, T2))f)
        (from, proxy_cast<T1>(args[0]), proxy_cast<T2>(args[1]));
}

template< typename T1, typename T2, typename T3 >
static void proxy(void (*f)(), Character *from, intptr_t const args[3])
{
    ((void (*)(Character *, T1, T2, T3))f)
        (from, proxy_cast<T1>(args[0]), proxy_cast<T2>(args[1]),
               proxy_cast<T3>(args[2]));
}

template< typename T1, typename T2, typename T3, typename T4 >
static void proxy(void (*f)(), Character *from, intptr_t const args[4])
{
    ((void (*)(Character *, T1, T2, T3, T4))f)
        (from, proxy_cast<T1>(args[0]), proxy_cast<T2>(args[1]),
               proxy_cast<T3>(args[2]), proxy_cast<T4>(args[3]));
}

/**
 * An argument type that a command can use.
 */

template< typename T > struct Argument;

template<> struct Argument< int >
{ static char const type = 'n'; };
template<> struct Argument< const std::string & >
{ static char const type = 's'; };
template<> struct Argument< Character * >
{ static char const type = 'c'; };
template<> struct Argument< MapComposite * >
{ static char const type = 'm'; };
template<> struct Argument< ItemClass * >
{ static char const type = 'i'; };
template<> struct Argument< MonsterClass * >
{ static char const type = 'o'; };

/**
 * A command that a user can run remotely with sufficient rights.
 */
struct Command
{
    const char *name;
    void (*handler)(void (*f)(), Character *, intptr_t const[]);
    void (*target)();
    char type[4];
    unsigned char level;
};

/**
 * Creates a command with a 1-parameter handler.
 */
template< typename T1 >
static Command handle(const char *name, int level,
                      void (*f)(Character *, T1))
{
    Command c;
    c.name = name;
    c.level = level;
    c.handler = &proxy< T1 >;
    c.target = (void (*)())f;
    c.type[0] = Argument<T1>::type;
    c.type[1] = 0;
    return c;
}

/**
 * Creates a command with a 2-parameter handler.
 */
template< typename T1, typename T2 >
static Command handle(const char *name, int level,
                      void (*f)(Character *, T1, T2))
{
    Command c;
    c.name = name;
    c.level = level;
    c.handler = &proxy< T1, T2 >;
    c.target = (void (*)())f;
    c.type[0] = Argument<T1>::type;
    c.type[1] = Argument<T2>::type;
    c.type[2] = 0;
    return c;
}

/**
 * Creates a command with a 3-parameter handler.
 */
template< typename T1, typename T2, typename T3 >
static Command handle(const char *name, int level,
                      void (*f)(Character *, T1, T2, T3))
{
    Command c;
    c.name = name;
    c.level = level;
    c.handler = &proxy< T1, T2, T3 >;
    c.target = (void (*)())f;
    c.type[0] = Argument<T1>::type;
    c.type[1] = Argument<T2>::type;
    c.type[2] = Argument<T3>::type;
    c.type[3] = 0;
    return c;
}

/**
 * Creates a command with a 4-parameter handler.
 */
template< typename T1, typename T2, typename T3, typename T4 >
static Command handle(const char *name, int level,
                      void (*f)(Character *, T1, T2, T3, T4))
{
    Command c;
    c.name = name;
    c.level = level;
    c.handler = &proxy< T1, T2, T3, T4 >;
    c.target = (void (*)())f;
    c.type[0] = Argument<T1>::type;
    c.type[1] = Argument<T2>::type;
    c.type[2] = Argument<T3>::type;
    c.type[3] = Argument<T4>::type;
    return c;
}

static void warp(Character *, Character *q, MapComposite *m, int x, int y)
{
    GameState::warp(q, m, x, y);
}

static void item(Character *, Character *q, ItemClass *it, int nb)
{
    Inventory(q).insert(it->getDatabaseID(), nb);
}

// This no longer works as money is now an attribute.
/*
static void money(Character *, Character *q, int nb)
{
    Inventory(q).changeMoney(nb);
}
*/

static void drop(Character *from, ItemClass *it, int nb)
{
    Item *item = new Item(it, nb);
    item->setMap(from->getMap());
    item->setPosition(from->getPosition());
    GameState::insertSafe(item);
}

static void spawn(Character *from, MonsterClass *specy, int nb)
{
    MapComposite *map = from->getMap();
    const Point &pos = from->getPosition();

    for (int i = 0; i < nb; ++i)
    {
        Being *monster = new Monster(specy);
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

static void goto_(Character *from, Character *ch)
{
    MapComposite *m = ch->getMap();
    const Point &pos = ch->getPosition();
    GameState::warp(from, m, pos.x, pos.y);
}

static void recall(Character *from, Character *ch)
{
    MapComposite *m = from->getMap();
    const Point &pos = from->getPosition();
    GameState::warp(ch, m, pos.x, pos.y);
}

static void reload(Character *, const std::string &db)
{
    if (db == "items")
    {
        itemManager->reload();
    }
    else if (db == "monsters")
    {
        monsterManager->reload();
    }
}

static void ban(Character *from, Character *ch, const std::string &duration)
{
    if (from->getAccountLevel() <= ch->getAccountLevel())
    {
        // Special case: Only ban strictly less priviledged accounts.
        return;
    }

    int d = atoi(duration.c_str());
    switch (duration[duration.length() - 1])
    {
        case 'd': d = d * 24; // nobreak
        case 'h': d = d * 60;
    }
    accountHandler->banCharacter(ch, d);
}

static void attribute(Character*, Character *ch, int attr, int value)
{
    ch->setAttribute(attr, value);
}

/**
 * List of remote commands.
 */
static Command const commands[] =
{
    handle("warp", AL_GM, warp),
    handle("item", AL_GM, item),
    handle("drop", AL_GM, drop),
//    handle("money", AL_GM, money),
    handle("spawn", AL_GM, spawn),
    handle("goto", AL_GM, goto_),
    handle("recall", AL_GM, recall),
    handle("reload", AL_ADMIN, reload),
    handle("ban", AL_GM, ban),
    handle("attribute", AL_GM, attribute),
};

/**
 * Send a message to the given character from the server.
 */
static void say(Character * ch, const std::string &message)
{
    GameState::sayTo(ch, NULL, message);
}


/**
 * Parses a command and executes its associated handler.
 */
void runCommand(Character *ch, const std::string &text)
{
    const Command *c = NULL;
    std::string::size_type npos = std::string::npos;
    std::string::size_type pos = text.find(' ');
    // remove the first letter which signifies it was a command
    std::string s(text, 1, pos == npos ? npos : pos - 1);

    // check for valid command
    for (int i = 0; i < (int)(sizeof(commands) / sizeof(commands[0])); ++i)
    {
        if (s == commands[i].name)
        {
            c = &commands[i];
            break;
        }
    }

    if (!c)
    {
        say(ch, "The command " + s + " was not found");
        return;
    }

    if (c->level > ch->getAccountLevel())
    {
        say(ch, "You have insufficient rights to perform the " + s + " command");
        return;
    }

    intptr_t args[4];

    for (int i = 0; i < 4 && c->type[i]; ++i)
    {
        if (pos == npos || pos + 1 >= text.length())
        {
            // Not enough parameters.
            say(ch, "Not enough parameters for the " + s +
                    " command. See the command documentation for details");
            return;
        }

        std::string::size_type pos2 = text.find(' ', pos + 1);
        std::string arg(text, pos + 1, pos2 == npos ? npos : pos2 - pos - 1);
        if (arg.empty())
        {
            // Empty parameter.
            say(ch, "One of your parameters was empty");
            return;
        }

        switch (c->type[i])
        {
            case 'c':
                if (arg == "#")
                {
                    // Character itself.
                    args[i] = (intptr_t)ch;
                }
                else
                {
                    GameClient *c = gameHandler->getClientByNameSlow(arg);
                    if (!c)
                    {
                        /* TODO: forward command to other game servers through
                           account server, in case the player is elsewhere. */
                        say(ch, "Player " + arg + " was not found");
                        return;
                    }
                    if (c->status != CLIENT_CONNECTED)
                    {
                        // No suitable character.
                        say(ch, "Player " + arg + " is offline");
                        return;
                    }
                    args[i] = (intptr_t)c->character;
                }
                break;

            case 'i':
                if (ItemClass *ic = itemManager->getItem(atoi(arg.c_str())))
                {
                    args[i] = (intptr_t)ic;
                }
                else
                {
                    // No such item.
                    say(ch, "No item was found with id " + arg);
                    return;
                }
                break;

            case 'm':
                if (arg == "#")
                {
                    // Map the character is on.
                    args[i] = (intptr_t)ch->getMap();
                }
                else if (MapComposite *m = MapManager::getMap(atoi(arg.c_str())))
                {
                    args[i] = (intptr_t)m;
                }
                else
                {
                    // No such map.
                    say(ch, "Map " + arg + " was not found");
                    return;
                }
                break;

            case 'n':
                args[i] = atoi(arg.c_str());
                break;

            case 'o':
                if (MonsterClass *mc = monsterManager->getMonster(atoi(arg.c_str())))
                {
                    args[i] = (intptr_t)mc;
                }
                else
                {
                    // No such item.
                    say(ch, "No monster with id " + arg + " was found");
                    return;
                }
                break;

            case 's':
                args[i] = (intptr_t)new std::string(arg);
                break;

        }
        pos = pos2;
    }

    // Call the command handler.
    c->handler(c->target, ch, args);

    // Delete dynamic arguments.
    for (int i = 0; i < 4 && c->type[i]; ++i)
    {
        if (c->type[i] == 's')
        {
            delete (std::string *)args[i];
        }
    }
}
