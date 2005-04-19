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

/*
 * ScriptingInterface provides a simple class which is a simple interface
 * for defining a scripting backend.
 */
class ScriptingInterface
{
  public:
    virtual ~ScriptingInterface() { };
    //Initialization
    virtual void init() = 0;
    //Destruction
    virtual void destroy() = 0;
    //State update
    virtual void update() = 0;
    //Script sent raw message
    virtual void message(char *) = 0;
};

extern ScriptingInterface *script;

#endif
