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

#ifndef LUASCRIPT_HPP
#define LUASCRIPT_HPP

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}

#include "scripting/script.hpp"

/**
 * Implementation of the Script class for Lua.
 */
class LuaScript: public Script
{
    public:
        /**
         * Constructor. Initializes a new Lua state, registers the native API
         * and loads the libmana.lua file.
         */
        LuaScript();

        /**
         * Destructor.
         */
        ~LuaScript();

        void load(const char *);

        void prepare(const std::string &);

        void push(int);

        void push(const std::string &);

        void push(Thing *);

        int execute();

        static void getQuestCallback(Character *, const std::string &,
                                     const std::string &, void *);

        static void getPostCallback(Character *, const std::string &,
                                    const std::string &, void *);

        void processDeathEvent(Being* thing);

        void processRemoveEvent(Thing* thing);

    private:

        lua_State *mState;
        int nbArgs;
        std::string mCurFunction;
};

static char const registryKey = 0;

static Script *LuaFactory()
{
    return new LuaScript();
}

struct LuaRegister
{
    LuaRegister() { Script::registerEngine("lua", LuaFactory); }
};

static LuaRegister dummy;

#endif // LUASCRIPT_HPP
