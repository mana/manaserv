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

#include "asscript.h"

//#include "luascript.h"

//#include "scripting/luautil.h"

//#include "game-server/being.h"
//#include "utils/logger.h"

//#include <cassert>
//#include <cstring>

AsScript::AsScript()
{
    // Create the AngelScript script engine
    asEngine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    if( asEngine == 0 )
    {
        LOG_FATAL("Failed to create Angel Script engine." << std::endl);
    }
    else
    {
        LOG_INFO("Angel Script engine successful created." << std::endl);
    }

    // AngelScript doesn't have a built-in string type, as there is no definite standard
    // string type for C++ applications. Every developer is free to register it's own string type.
    // The SDK do however provide a standard add-on for registering a string type, so it's not
    // necessary to implement the registration yourself if you don't want to.
    RegisterStdString(asEngine);

    // Register the function that we want the scripts to call
    //int r=asEngine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL); assert( r >= 0 );
}

AsScript::~AsScript()
{
    // Release the engine
    asEngine->Release();
}

void AsScript::load(const char *prog, const char *name)
{
       int nRet=builder.StartNewModule(asEngine, name);

       if( nRet < 0 )
       {
         // If the code fails here it is usually because there
         // is no more memory to allocate the module
         LOG_FATAL("Unrecoverable error while starting a new module.");
         return;
       }

      nRet=builder.AddSectionFromMemory(prog, name);

      if( nRet < 0 )
      {
        // The builder wasn't able to load the file. Maybe the file
        // has been removed, or the wrong name was given, or some
        // preprocessing commands are incorrectly written.
        LOG_FATAL("Please correct the errors in the script and try again.");
        return;
      }

      nRet = builder.BuildModule();
      if( nRet < 0 )
      {
        // An error occurred. Instruct the script writer to fix the
        // compilation errors that were listed in the output stream.
        LOG_FATAL("Please correct the errors in the script and try again.\n");
        return;
      }
}

void AsScript::prepare(const std::string &name)
{
    //assert(nbArgs == -1);
//    lua_getglobal(mState, name.c_str());
//    nbArgs = 0;
//    mCurFunction = name;
}

void AsScript::push(int v)
{
//    assert(nbArgs >= 0);
//    lua_pushinteger(mState, v);
//    ++nbArgs;
}

void AsScript::push(const std::string &v)
{
//    assert(nbArgs >= 0);
//    lua_pushstring(mState, v.c_str());
//    ++nbArgs;
}

void AsScript::push(Thing *v)
{
//    assert(nbArgs >= 0);
//    lua_pushlightuserdata(mState, v);
//    ++nbArgs;
}

void AsScript::push(const std::list<InventoryItem> &itemList)
{
//    assert(nbArgs >= 0);
//    int position = 0;

//    lua_createtable(mState, itemList.size(), 0);
//    int itemTable = lua_gettop(mState);

//    for (std::list<InventoryItem>::const_iterator i = itemList.begin();
//         i != itemList.end();
//         i++)
//    {
//        // create the item structure
//        std::map<std::string, int> item;
//        item["id"] = i->itemId;
//        item["amount"] = i->amount;
//        // add the item structure to the item table under the next index
//        lua_pushinteger(mState, ++position);
//        pushSTLContainer<std::string, int>(mState, item);
//        lua_settable(mState, itemTable);
//    }
//    ++nbArgs;
}

int AsScript::execute()
{
//    assert(nbArgs >= 0);
//    int res = lua_pcall(mState, nbArgs, 1, 1);
//    nbArgs = -1;
//    if (res || !(lua_isnil(mState, -1) || lua_isnumber(mState, -1)))
//    {
//        const char *s = lua_tostring(mState, -1);

//        LOG_WARN("Lua Script Error" << std::endl
//                 << "     Script  : " << mScriptFile << std::endl
//                 << "     Function: " << mCurFunction << std::endl
//                 << "     Error   : " << (s ? s : "") << std::endl);
//        lua_pop(mState, 1);
//        return 0;
//    }
//    res = lua_tointeger(mState, -1);
//    lua_pop(mState, 1);
//    return res;
//    mCurFunction.clear();

    return 0;
}

void AsScript::processDeathEvent(Being *being)
{
//    prepare("death_notification");
//    push(being);
//    //TODO: get and push a list of creatures who contributed to killing the
//    //      being. This might be very interesting for scripting quests.
//    execute();
}

void AsScript::processRemoveEvent(Thing *being)
{
//    prepare("remove_notification");
//    push(being);
//    //TODO: get and push a list of creatures who contributed to killing the
//    //      being. This might be very interesting for scripting quests.
//    execute();

//    being->removeListener(getScriptListener());
}
