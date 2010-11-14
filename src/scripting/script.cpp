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

#include "scripting/script.h"

#include "common/resourcemanager.h"
#include "game-server/being.h"
#include "utils/logger.h"

#include <cstdlib>
#include <map>

#include <string.h>

typedef std::map< std::string, Script::Factory > Engines;

static Engines *engines = NULL;
Script *Script::globalEventScript = NULL;
Script *Script::specialActionsScript = NULL;

Script::Script():
    mMap(NULL),
    mEventListener(&scriptEventDispatch)
{}

void Script::registerEngine(const std::string &name, Factory f)
{
    if (!engines)
    {
        /* The "engines" variable is a pointer instead of an object, because
           this file may not have been initialized at the time this function
           is called. So we take care of the initialization by hand, in order
           to ensure we will not segfault at startup. */
        engines = new Engines;
    }
    (*engines)[name] = f;
}

Script *Script::create(const std::string &engine)
{
    if (engines)
    {
        Engines::const_iterator i = engines->find(engine);
        if (i != engines->end())
        {
            return i->second();
        }
    }
    LOG_ERROR("No scripting engine named " << engine);
    return NULL;
}

void Script::update()
{
    prepare("update");
    execute();
}

static char *skipPotentialBom(char *text)
{
    // Based on the C version of bomstrip
    const char * const utf8Bom = "\xef\xbb\xbf";
    const int bomLength = strlen(utf8Bom);
    return (strncmp(text, utf8Bom, bomLength) == 0) ? text + bomLength : text;
}

bool Script::loadFile(const std::string &name)
{
    int size;
    char *buffer = ResourceManager::loadFile(name, size);
    if (buffer)
    {
        mScriptFile = name;
        load(skipPotentialBom(buffer), name.c_str());
        free(buffer);
        return true;
    } else {
        return false;
    }
}

void Script::loadNPC(const std::string &name, int id, int x, int y,
                     const char *prog)
{
    load(prog, name.c_str());
    prepare("create_npc_delayed");
    push(name);
    push(id);
    push(x);
    push(y);
    execute();
}

bool Script::executeGlobalEventFunction(const std::string &function, Being* obj)
{
    bool isScriptHandled = false;
    Script *script = Script::globalEventScript;
    if (script)
    {
        script->setMap(obj->getMap());
        script->prepare(function);
        script->push(obj);
        script->execute();
        script->setMap(NULL);
        isScriptHandled = true; // TODO: don't set to true when execution failed
    }
    return isScriptHandled;
}


void Script::addDataToSpecial(int id, Special* special)
{
    /* currently only gets the recharge cost.
      TODO: get any other info in a similar way, but
            first we have to agree on what other
            info we actually want to provide.
    */
    if (special)
    {
        Script *script = Script::specialActionsScript;
        if (script)
        {
            script->prepare("get_special_recharge_cost");
            script->push(id);
            int scriptReturn = script->execute();
            special->neededMana = scriptReturn;
        }
    }

}

bool Script::performSpecialAction(int specialId, Being* caster)
{
    Script *script = Script::specialActionsScript;
    if (script)
    {
        script->prepare("use_special");
        script->push(caster);
        script->push(specialId);
        script->execute();
    }
    return true;
}
