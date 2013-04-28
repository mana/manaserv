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

#include "scripting/script.h"

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}

#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "game-server/attack.h"
#include "game-server/specialmanager.h"

class CharacterComponent;
class Entity;
class ItemClass;
class MapComposite;
class MapObject;
class MonsterClass;
class StatusEffect;

void raiseWarning(lua_State *s, const char *format, ...);

/**
 * A generic userdata cache based on a native Lua table with weak values.
 *
 * Caching the created userdata instances has two main advantages:
 *
 * - It limits memory consumption and is a little faster. Creating these values
 *   is relatively slow (compared to light userdata).
 *
 * - It makes sure that two userdata objects that refer to the same C++ object
 *   compare as equal (because they will be the same userdata object).
 */
class UserDataCache
{
public:
    /**
     * Attempts to retrieve a userdata associated with the given object from
     * the cache and pushes it on the stack when available.
     *
     * Returns whether a userdata was pushed.
     */
    static bool retrieve(lua_State *s, void *object);

    /**
     * Inserts the userdata at the top of the stack in the cache using
     * the given object as the key. Leaves the userdata on the stack.
     */
    static void insert(lua_State *s, void *object);

private:
    static char mRegistryKey;
};


/**
 * A helper class for pushing and checking custom Lua userdata types.
 */
template <typename T>
class LuaUserData
{
public:
    /**
     * Creates a metatable to be used for the userdata associated with the
     * type. Then, registers the \a members with a library named \a typeName,
     * and sets the '__index' member of the metatable to this library.
     */
    static void registerType(lua_State *s,
                             const char *typeName,
                             const luaL_Reg *members)
    {
        mTypeName = typeName;

        luaL_newmetatable(s, mTypeName);        // metatable
        lua_pushliteral(s, "__index");          // metatable, "__index"
        lua_createtable(s, 0, 0);               // metatable, "__index", {}
#if LUA_VERSION_NUM < 502
        luaL_register(s, nullptr, members);
#else
        luaL_setfuncs(s, members, 0);
#endif
        lua_rawset(s, -3);                      // metatable
        lua_pop(s, 1);                          // -empty-
    }

    /**
     * Pushes a userdata reference to the given object on the stack. Either by
     * creating one, or reusing an existing one.
     *
     * When a null-pointer is passed for \a object, the value 'nil' is pushed.
     */
    static void push(lua_State *s, T *object)
    {
        if (!object)
        {
            lua_pushnil(s);
        }
        else if (!UserDataCache::retrieve(s, object))
        {
            void *userData = lua_newuserdata(s, sizeof(T*));
            * static_cast<T**>(userData) = object;

#if LUA_VERSION_NUM < 502
            luaL_newmetatable(s, mTypeName);
            lua_setmetatable(s, -2);
#else
            luaL_setmetatable(s, mTypeName);
#endif

            UserDataCache::insert(s, object);
        }
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

typedef LuaUserData<AttackInfo> LuaAttackInfo;
typedef LuaUserData<Damage> LuaDamage;
typedef LuaUserData<Entity> LuaEntity;
typedef LuaUserData<ItemClass> LuaItemClass;
typedef LuaUserData<MapObject> LuaMapObject;
typedef LuaUserData<MonsterClass> LuaMonsterClass;
typedef LuaUserData<StatusEffect> LuaStatusEffect;
typedef LuaUserData<SpecialManager::SpecialInfo> LuaSpecialInfo;

Script *        getScript(lua_State *s);

ItemClass *     getItemClass(lua_State *s, int p);
MonsterClass *  getMonsterClass(lua_State *s, int p);

Entity *        checkActor(lua_State *s, int p);
Entity *        checkBeing(lua_State *s, int p);
Entity *        checkCharacter(lua_State *s, int p);
ItemClass *     checkItemClass(lua_State *s, int p);
Entity *        checkMonster(lua_State *s, int p);
MonsterClass *  checkMonsterClass(lua_State *s, int p);
Entity *        checkNpc(lua_State *s, int p);
int             checkSkill(lua_State *s, int p);
int             checkSpecial(lua_State *s, int p);

MapComposite *  checkCurrentMap(lua_State *s, Script *script = 0);
Script::Thread* checkCurrentThread(lua_State *s, Script *script = 0);


/* Polymorphic wrapper for pushing variables.
   Useful for templates.*/

inline void push(lua_State *s, int val)
{
    lua_pushinteger(s, val);
}

inline void push(lua_State *s, const std::string &val)
{
    lua_pushlstring(s, val.c_str(), val.length());
}

inline void push(lua_State *s, Entity *val)
{
    LuaEntity::push(s, val);
}

inline void push(lua_State *s, double val)
{
    lua_pushnumber(s, val);
}

inline void push(lua_State *s, AttackInfo *val)
{
    LuaAttackInfo::push(s, val);
}

inline void push(lua_State *s, MapObject *val)
{
    LuaMapObject::push(s, val);
}


/*  Pushes an STL LIST */
template <typename T>
void pushSTLContainer(lua_State *s, const std::list<T> &container)
{
    int len = container.size();
    lua_createtable(s, len, 0);
    int table = lua_gettop(s);
    typename std::list<T>::const_iterator i;
    i = container.begin();

    for (int key = 1; key <= len; key++)
    {
        push(s, *i);
        lua_rawseti(s, table, key);
        ++i;
    }
}

/*  Pushes an STL VECTOR */
template <typename T>
void pushSTLContainer(lua_State *s, const std::vector<T> &container)
{
    int len = container.size();
    lua_createtable(s, len, 0);
    int table = lua_gettop(s);

    for (int key = 0; key < len; key++)
    {
        push(s, container.at(key));
        lua_rawseti(s, table, key + 1);
    }
}

/*  Pushes an STL MAP */
template <typename Tkey, typename Tval>
void pushSTLContainer(lua_State *s, const std::map<Tkey, Tval> &container)
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
        ++i;
    }
}

/*  Pushes an STL SET */
template <typename T>
void pushSTLContainer(lua_State *s, const std::set<T> &container)
{
    int len = container.size();
    lua_createtable(s, len, 0);
    int table = lua_gettop(s);
    typename std::set<T>::const_iterator i;
    i = container.begin();

    for (int key = 1; key <= len; key++)
    {
        push(s, *i);
        lua_rawseti(s, table, key);
        ++i;
    }
}

#endif
