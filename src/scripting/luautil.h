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

#ifndef SCRIPTING_LUAUTIL_H
#define SCRIPTING_LUAUTIL_H

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}
#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "game-server/map.h"

class Being;
class NPC;
class Character;
class Monster;
class Thing;

// Report script errors and interrupt the script.
void raiseScriptError(lua_State *s, const char *format, ...);

void raiseWarning(lua_State *s, const char *format, ...);

/**
 * A helper class for pushing and checking custom Lua user data types.
 */
template <typename T>
class LuaUserData
{
public:
    /**
     * Creates a metatable to be used for the user data associated with the
     * type. Then, registers the \a members with a library named \a typeName,
     * and sets the '__index' member of the metatable to this library.
     */
    static void registerType(lua_State *s,
                             const char *typeName,
                             const luaL_Reg *members)
    {
        mTypeName = typeName;

        luaL_newmetatable(s, mTypeName);        // metatable
        lua_pushstring(s, "__index");           // metatable, "__index"
        luaL_register(s, typeName, members);    // metatable, "__index", {}
        lua_rawset(s, -3);                      // metatable
        lua_pop(s, 1);                          // -empty-
    }

    /**
     * Pushes a userdata reference to the given object on the stack. Either by
     * creating one, or reusing an existing one.
     */
    static int push(lua_State *L, T *object)
    {
        T **userData = static_cast<T**>(lua_newuserdata(L, sizeof(T*)));
        *userData = object;

        luaL_newmetatable(L, mTypeName);
        lua_setmetatable(L, -2);

        return 1;
    }

    /**
     * Returns the argument at position \a narg when it is of the right type,
     * and raises a Lua error otherwise.
     */
    static T *check(lua_State *L, int narg)
    {
        void *userData = luaL_checkudata(L, narg, mTypeName);
        return *(static_cast<T**>(userData));
    }

private:
    static const char *mTypeName;
};

template <typename T> const char * LuaUserData<T>::mTypeName;

typedef LuaUserData<MapObject> LuaMapObject;


NPC *getNPC(lua_State *s, int p);
Character *getCharacter(lua_State *s, int p);
Monster *getMonster(lua_State *s, int p);
Being *getBeing(lua_State *s, int p);

/* Polymorphic wrapper for pushing variables.
   Useful for templates.*/
void push(lua_State *s, int val);
void push(lua_State *s, const std::string &val);
void push(lua_State *s, Thing *val);
void push(lua_State *s, double val);

inline void push(lua_State *s, MapObject *val)
{
    LuaMapObject::push(s, val);
}


/*  Pushes an STL LIST */
template <typename T> void pushSTLContainer(lua_State *s, const std::list<T> &container)
{
    int len = container.size();
    lua_createtable(s, len, 0);
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
    lua_createtable(s, len, 0);
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
    lua_createtable(s, len, 0);
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
