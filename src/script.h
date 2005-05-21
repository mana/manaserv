/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
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

#ifndef SCRIPT_H
#define SCRIPT_H

#ifdef SCRIPT_SUPPORT

#include <iostream>

/*
 * Script
 * Script provides a simple class which is a simple interface
 * for defining a scripting backend. Each Script object consists
 * of an independant scripting environment.
 *
 * Requirements of Script Object:
 * - Each script object is independant from any other script object.
 * - A script is to be executed upon instantiation.
 * - A initialization and destruction function should be executed in
 *   the script if such a function is defined (the interface should
 *   provide default methods in case they are missing or not needed.)
 */
class Script
{
  protected:
    // Filename of the script corresponding to the script object
    std::string scriptName;

  public:
    Script(const std::string &file)
	: scriptName(file)
    { }

    virtual ~Script() { }

    //State update
    virtual void update() = 0;

    //Execute specified function
    virtual bool execute(const std::string &) = 0;

    //Script sent raw message
    virtual void message(char *) = 0;
};

extern Script *script;                  // Global script (temporary?

#endif

#endif
