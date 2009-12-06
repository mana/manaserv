/*
 *  The Mana Server
 *  Copyright (C) 2007  The Mana World Development Team
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

#ifndef SCRIPTING_LUAUTIL_HPP
#define SCRIPTING_LUAUTIL_HPP

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}
#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>

class Being;
class NPC;
class Character;
class Thing;

void raiseScriptError(lua_State *s, const char *format, ...);

NPC *getNPC(lua_State *s, int p);
Character *getCharacter(lua_State *s, int p);
Being *getBeing(lua_State *s, int p);


/* Polymorphic wrapper for pushing variables.
   Useful for templates.*/
void push(lua_State *s, int val);
void push(lua_State *s, const std::string &val);
void push(lua_State *s, Thing* val);
void push(lua_State *s, double val);


/*  Pushes an STL LIST */
template <typename T> void pushSTLContainer(lua_State *s, const std::list<T> &container)
{
    int len = container.size();
    lua_newtable(s);
    int table = lua_gettop(s);
    typename std::list<T>::const_iterator i;
    i = container.begin();

    for (int key = 1; key <= len; key++)
    {
        push(s, key);
        push(s, *i);
        lua_settable(s, table);
        i++;
    }
}

/*  Pushes an STL VECTOR */
template <typename T> void pushSTLContainer(lua_State *s, const std::vector<T> &container)
{
    int len = container.size();
    lua_createtable(s, 0, len);
    int table = lua_gettop(s);

    for (int key = 0; key < len; key++)
    {
        push(s, key+1);
        push(s, container.at(key));
        lua_settable(s, table);
    }
}

/*  Pushes an STL MAP */
template <typename Tkey, typename Tval> void pushSTLContainer(lua_State *s, const std::map<Tkey, Tval> &container)
{
    int len = container.size();
    lua_createtable(s, 0, len);
    int table = lua_gettop(s);
    typename std::map<Tkey, Tval>::const_iterator i;
    i = container.begin();

    for (int key = 1; key <= len; key++)
    {
        push(s, i->first);
        push(s, i->second);
        lua_settable(s, table);
        i++;
    }
}

/*  Pushes an STL SET */
template <typename T> void pushSTLContainer(lua_State *s, const std::set<T> &container)
{
    int len = container.size();
    lua_newtable(s);
    int table = lua_gettop(s);
    typename std::set<T>::const_iterator i;
    i = container.begin();

    for (int key = 1; key <= len; key++)
    {
        push(s, key);
        push(s, *i);
        lua_settable(s, table);
        i++;
    }
}

#endif
