/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
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

#include <cstddef>

#include "defines.h"
#include "game-server/character.hpp"
#include "game-server/inventory.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/state.hpp"

template< typename T1 >
static void proxy(void (*f)(), Character *from, intptr_t args[1])
{
    ((void (*)(Character *, T1))f)
        (from, (T1)args[0]);
}

template< typename T1, typename T2 >
static void proxy(void (*f)(), Character *from, intptr_t args[2])
{
    ((void (*)(Character *, T1, T2))f)
        (from, (T1)args[0], (T2)args[1]);
}

template< typename T1, typename T2, typename T3 >
static void proxy(void (*f)(), Character *from, intptr_t args[3])
{
    ((void (*)(Character *, T1, T2, T3))f)
        (from, (T1)args[0], (T2)args[1], (T3)args[2]);
}

template< typename T1, typename T2, typename T3, typename T4 >
static void proxy(void (*f)(), Character *from, intptr_t args[4])
{
    ((void (*)(Character *, T1, T2, T3, T4))f)
        (from, (T1)args[0], (T2)args[1], (T3)args[2], (T4)args[3]);
}

/**
 * An argument type that a command can use.
 */

template< typename T > struct Argument;

template<> struct Argument< int >
{ static char const type = 'n'; };
template<> struct Argument< Character * >
{ static char const type = 'c'; };
template<> struct Argument< MapComposite * >
{ static char const type = 'm'; };
template<> struct Argument< ItemClass * >
{ static char const type = 'i'; };

/**
 * A command that a user can run remotely with sufficient rights.
 */
struct Command
{
    char const *name;
    char type[4];
    void (*handler)(void (*f)(), Character *, intptr_t[]);
    void (*target)();
    unsigned char level;
};

/**
 * Creates a command with a 1-parameter handler.
 */
template< typename T1 >
static Command handle(char const *name, int level,
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
static Command handle(char const *name, int level,
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
static Command handle(char const *name, int level,
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
static Command handle(char const *name, int level,
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
    DelayedEvent e = { EVENT_WARP, x, y, m };
    GameState::enqueueEvent(q, e);
}

static void item(Character *, Character *q, ItemClass *it, int nb)
{
    Inventory(q).insert(it->getDatabaseID(), nb);
}

static void money(Character *, Character *q, int nb)
{
    Inventory(q).changeMoney(nb);
}

static void drop(Character *from, ItemClass *it, int nb)
{
    Item *item = new Item(it, nb);
    item->setMap(from->getMap());
    item->setPosition(from->getPosition());
    DelayedEvent e = { EVENT_INSERT };
    GameState::enqueueEvent(item, e);
}

/**
 * List of remote commands.
 */
static Command const commands[] =
{
    handle("warp", AL_GM, warp),
    handle("item", AL_GM, item),
    handle("drop", AL_GM, drop),
    handle("money", AL_GM, money),
};

/**
 * Parses a command and executes its associated handler.
 */
void runCommand(Character *ch, std::string const &text)
{
    Command const *c = NULL;
    std::string::size_type npos = std::string::npos;
    std::string::size_type pos = text.find(' ');
    std::string s(text, 1, pos == npos ? npos : pos - 1); // Skip slash.

    for (int i = 0; i < (int)(sizeof(commands) / sizeof(commands[0])); ++i)
    {
        if (s == commands[i].name)
        {
            c = &commands[i];
            break;
        }
    }

    if (!c || c->level < ch->getAccountLevel())
    {
        // No such command or no sufficient rights.
        return;
    }

    intptr_t args[4];

    for (int i = 0; i < 4 && c->type[i]; ++i)
    {
        if (pos == npos || pos + 1 >= text.length())
        {
            // Not enough parameters.
            return;
        }

        std::string::size_type pos2 = text.find(' ', pos + 1);
        std::string arg(text, pos + 1, pos2 == npos ? npos : pos2 - pos - 1);
        if (arg.empty())
        {
            // Empty parameter.
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
                    // TODO: explicitly named character.
                    return;
                }
                break;

            case 'i':
                if (ItemClass *ic = ItemManager::getItem(atoi(arg.c_str())))
                {
                    args[i] = (intptr_t)ic;
                }
                else
                {
                    // No such item.
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
                    return;
                }
                break;                

            case 'n':
                args[i] = atoi(arg.c_str());
                break;
        }
        pos = pos2;
    }
    c->handler(c->target, ch, args);
}
