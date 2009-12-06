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

#include "luautil.hpp"

#include "game-server/character.hpp"
#include "game-server/npc.hpp"

#include "utils/logger.h"


void raiseScriptError(lua_State *s, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char message[1024];
    vsprintf(message, format, args);
    va_end( args );

    LOG_WARN("Lua script error: "<<message);
    luaL_error(s, message);
}

/* Functions below are unsafe, as they assume the script has passed pointers
   to objects which have not yet been destroyed. If the script never keeps
   pointers around, there will be no problem. In order to be safe, the engine
   should replace pointers by local identifiers and store them in a map. By
   listening to the death of objects, it could keep track of pointers still
   valid in the map.
   TODO: do it. */

NPC *getNPC(lua_State *s, int p)
{
    if (!lua_islightuserdata(s, p)) return NULL;
    Thing *t = static_cast<Thing *>(lua_touserdata(s, p));
    if (t->getType() != OBJECT_NPC) return NULL;
    return static_cast<NPC *>(t);
}

Character *getCharacter(lua_State *s, int p)
{
    if (!lua_islightuserdata(s, p)) return NULL;
    Thing *t = static_cast<Thing *>(lua_touserdata(s, p));
    if (t->getType() != OBJECT_CHARACTER) return NULL;
    return static_cast<Character *>(t);
}

Being *getBeing(lua_State *s, int p)
{
    if (!lua_islightuserdata(s, p)) return NULL;
    Thing *t = static_cast<Thing *>(lua_touserdata(s, p));
    return static_cast<Being *>(t);
}

void push(lua_State *s, int val)
{
    lua_pushinteger(s, val);
}

void push(lua_State *s, const std::string &val)
{
    lua_pushstring(s, val.c_str());
}

void push(lua_State *s, Thing* val)
{
    lua_pushlightuserdata(s, val);
}

void push(lua_State *s, double val)
{
    lua_pushnumber(s, val);
}
