/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
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

#include "luautil.h"

#include "game-server/character.h"
#include "game-server/itemmanager.h"
#include "game-server/monster.h"
#include "game-server/monstermanager.h"
#include "game-server/npc.h"
#include "game-server/skillmanager.h"

#include "utils/logger.h"

#include "scripting/luascript.h"


void raiseWarning(lua_State *, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char message[1024];
    vsprintf(message, format, args);
    va_end( args );

    LOG_WARN("Lua script error: "<< message);
}


char UserDataCache::mRegistryKey;

bool UserDataCache::retrieve(lua_State *s, void *object)
{
    // Retrieve the cache table
    lua_pushlightuserdata(s, &mRegistryKey);    // key
    lua_rawget(s, LUA_REGISTRYINDEX);           // Cache?

    if (lua_isnil(s, -1))
    {
        lua_pop(s, 1);
        return false;
    }

    lua_pushlightuserdata(s, object);           // Cache, object
    lua_rawget(s, -2);                          // Cache, UD?

    if (lua_isnil(s, -1))
    {
        lua_pop(s, 2);                          // ...
        return false;
    }

    lua_replace(s, -2);                         // UD
    return true;
}

void UserDataCache::insert(lua_State *s, void *object)
{
    // Retrieve the cache table
    lua_pushlightuserdata(s, &mRegistryKey);    // UD, key
    lua_rawget(s, LUA_REGISTRYINDEX);           // UD, Cache?

    // Create the cache when it doesn't exist yet
    if (lua_isnil(s, -1))
    {
        lua_pop(s, 1);                          // UD
        lua_newtable(s);                        // UD, Cache

        // The metatable that makes the values in the table above weak
        lua_newtable(s);                        // UD, Cache, {}
        lua_pushliteral(s, "__mode");
        lua_pushliteral(s, "v");
        lua_rawset(s, -3);                      // UD, Cache, { __mode = "v" }
        lua_setmetatable(s, -2);                // UD, Cache

        lua_pushlightuserdata(s, &mRegistryKey);// UD, Cache, key
        lua_pushvalue(s, -2);                   // UD, Cache, key, Cache
        lua_rawset(s, LUA_REGISTRYINDEX);       // UD, Cache
    }

    lua_pushlightuserdata(s, object);           // UD, Cache, object
    lua_pushvalue(s, -3);                       // UD, Cache, object, UD
    lua_rawset(s, -3);                          // UD, Cache { object = UD }
    lua_pop(s, 1);                              // UD
}


Script *getScript(lua_State *s)
{
    lua_pushlightuserdata(s, (void *)&LuaScript::registryKey);
    lua_gettable(s, LUA_REGISTRYINDEX);
    Script *script = static_cast<Script *>(lua_touserdata(s, -1));
    lua_pop(s, 1);
    return script;
}


/* Functions below are unsafe, as they assume the script has passed pointers
   to objects which have not yet been destroyed. If the script never keeps
   pointers around, there will be no problem. In order to be safe, the engine
   should replace pointers by local identifiers and store them in a map. By
   listening to the death of objects, it could keep track of pointers still
   valid in the map.
   TODO: do it. */

ItemClass *getItemClass(lua_State *s, int p)
{
    ItemClass *itemClass = 0;

    switch (lua_type(s, p))
    {
    case LUA_TNUMBER:
        itemClass = itemManager->getItem(lua_tointeger(s, p));
        break;
    case LUA_TSTRING:
        itemClass = itemManager->getItemByName(lua_tostring(s, p));
        break;
    case LUA_TUSERDATA:
        itemClass = LuaItemClass::check(s, p);
        break;
    }

    return itemClass;
}

MonsterClass *getMonsterClass(lua_State *s, int p)
{
    MonsterClass *monsterClass = 0;

    switch (lua_type(s, p))
    {
    case LUA_TNUMBER:
        monsterClass = monsterManager->getMonster(lua_tointeger(s, p));
        break;
    case LUA_TSTRING:
        monsterClass = monsterManager->getMonsterByName(lua_tostring(s, p));
        break;
    case LUA_TUSERDATA:
        monsterClass = LuaMonsterClass::check(s, p);
        break;
    }

    return monsterClass;
}


Entity *checkActor(lua_State *s, int p)
{
    Entity *entity = LuaEntity::check(s, p);
    luaL_argcheck(s, entity->hasComponent<ActorComponent>(), p,
                  "entity has no actor component");
    return entity;
}

Entity *checkBeing(lua_State *s, int p)
{
    Entity *entity = LuaEntity::check(s, p);
    luaL_argcheck(s, entity->hasComponent<BeingComponent>(), p,
                  "entity has no being component");
    return entity;
}

Entity *checkCharacter(lua_State *s, int p)
{
    Entity *entity = LuaEntity::check(s, p);
    luaL_argcheck(s, entity->getType() == OBJECT_CHARACTER, p, "character expected");
    return entity;
}

ItemClass *checkItemClass(lua_State *s, int p)
{
    ItemClass *itemClass = getItemClass(s, p);
    luaL_argcheck(s, itemClass, p, "item type expected");
    return itemClass;
}

Entity *checkMonster(lua_State *s, int p)
{
    Entity *entity = LuaEntity::check(s, p);
    luaL_argcheck(s, entity->getType() == OBJECT_MONSTER, p, "monster expected");
    return entity;
}

MonsterClass *checkMonsterClass(lua_State *s, int p)
{
    MonsterClass *monsterClass = getMonsterClass(s, p);
    luaL_argcheck(s, monsterClass, p, "monster type expected");
    return monsterClass;
}

Entity *checkNpc(lua_State *s, int p)
{
    Entity *entity = LuaEntity::check(s, p);
    luaL_argcheck(s, entity->getType() == OBJECT_NPC, p, "npc expected");
    return entity;
}

int checkSkill(lua_State *s, int p)
{
    if (lua_isnumber(s, p))
        return luaL_checkint(s, p);

    int id = skillManager->getId(luaL_checkstring(s, p));
    luaL_argcheck(s, id != 0, p, "invalid skill name");
    return id;
}

int checkAbility(lua_State *s, int p)
{
    if (lua_isnumber(s, p))
        return luaL_checkint(s, p);

    int id = abilityManager->getId(luaL_checkstring(s, p));
    luaL_argcheck(s, id != 0, p, "invalid ability name");
    return id;
}


MapComposite *checkCurrentMap(lua_State *s, Script *script /* = 0 */)
{
    if (!script)
        script = getScript(s);

    MapComposite *mapComposite = script->getContext()->map;
    if (!mapComposite)
        luaL_error(s, "no current map");

    return mapComposite;
}

Script::Thread *checkCurrentThread(lua_State *s, Script *script /* = 0 */)
{
    if (!script)
        script = getScript(s);

    Script::Thread *thread = script->getCurrentThread();
    if (!thread)
        luaL_error(s, "function requires threaded execution");

    return thread;
}
