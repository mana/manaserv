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

#ifndef LUASCRIPT_H
#define LUASCRIPT_H

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}

#include "scripting/script.h"

/**
 * Implementation of the Script class for Lua.
 */
class LuaScript : public Script
{
    public:
        /**
         * Constructor. Initializes a new Lua state, registers the native API
         * and loads the libmana.lua file.
         */
        LuaScript();

        ~LuaScript();

        void load(const char *prog, const char *name);

        void prepare(Ref function);

        void push(int);

        void push(const std::string &);

        void push(Thing *);

        void push(const std::list<InventoryItem> &itemList);

        int execute();

        void assignCallback(Ref &function);

        static void getQuestCallback(Character *, const std::string &,
                                     const std::string &, Script *);

        static void getPostCallback(Character *, const std::string &,
                                    const std::string &, Script *);

        void processDeathEvent(Being *thing);

        void processRemoveEvent(Thing *thing);


        static void setQuestReplyCallback(Script *script)
        { script->assignCallback(mQuestReplyCallback); }

        static void setPostReplyCallback(Script *script)
        { script->assignCallback(mPostReplyCallback); }

        static void setDeathNotificationCallback(Script *script)
        { script->assignCallback(mDeathNotificationCallback); }

        static void setRemoveNotificationCallback(Script *script)
        { script->assignCallback(mRemoveNotificationCallback); }

    private:
        lua_State *mState;
        int nbArgs;

        static Ref mQuestReplyCallback;
        static Ref mPostReplyCallback;
        static Ref mDeathNotificationCallback;
        static Ref mRemoveNotificationCallback;
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

#endif // LUASCRIPT_H
