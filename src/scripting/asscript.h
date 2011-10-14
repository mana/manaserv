/*
 *  The Mana Server
 *  Copyright (C) 2007-2011  The Mana World Development Team
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

#ifndef ASSCRIPT_H
#define ASSCRIPT_H

#include <angelscript.h>
#include "scripting/angelscript/scriptstdstring/scriptstdstring.h"
#include "scripting/angelscript/scriptbuilder/scriptbuilder.h"

#include "scripting/script.h"

///**
// * Implementation of the Script class for Lua.
// */
class AsScript: public Script
{
    public:
        AsScript();

        ~AsScript();

        void load(const char *prog, const char *name);

        void prepare(const std::string &);

        void push(int);

        void push(const std::string &);

        void push(Thing *);

        void push(const std::list<InventoryItem> &itemList);

        int execute();

        void processDeathEvent(Being *thing);

        void processRemoveEvent(Thing *thing);

        //Script bindings and helper functions
        //void raiseScriptError(const char* description);

        //int log(const int logLevel);

    private:
        void executeScript();

        asIScriptEngine * asEngine;
        asIScriptContext *asContext;
        int nbArgs;

        // The CScriptBuilder helper is an add-on that loads the file,
        // performs a pre-processing pass if necessary, and then tells
        // the engine to build a script module.
        CScriptBuilder builder;
//        lua_State *mState;
//        int nbArgs;
//        std::string mCurFunction;
};

static char const registryKey = 0;

static Script *AsFactory()
{
    return new AsScript();
}

struct AsRegister
{
    AsRegister() { Script::registerEngine("angelscript", AsFactory); }
};

static AsRegister dummy;

#endif // ASSCRIPT_H
