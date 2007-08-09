/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#include <map>

#include "utils/logger.h"
#include "scripting/script.hpp"

typedef std::map< std::string, Script::Factory > Engines;

static Engines *engines = NULL;

void Script::registerEngine(std::string const &name, Factory f)
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

Script *Script::create(std::string const &engine, std::string const &file)
{
    Engines::const_iterator i = engines->find(engine);
    if (i == engines->end())
    {
        LOG_ERROR("No scripting engine named " << engine);
        return NULL;
    }
    Script *s = i->second(file);
    if (!s)
    {
        LOG_ERROR("Failure while loading script " << file);
    }
    return s;
}

void Script::update()
{
    prepare("update");
    execute();
}

