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

#ifndef _TMWSERV_SCRIPTING_SCRIPT_HPP
#define _TMWSERV_SCRIPTING_SCRIPT_HPP

#include <string>

class MapComposite;
class Thing;

/**
 * Abstract interface for calling functions written in an external language.
 */
class Script
{
    public:

        typedef Script *(*Factory)(std::string const &);

        /**
         * Registers a new scripting engine.
         */
        static void registerEngine(std::string const &, Factory);

        /**
         * Creates a new script.
         */
        static Script *create(std::string const &engine, std::string const &file);

        Script(): mMap(NULL) {}

        virtual ~Script() {}

        /**
         * Called every tick for the script to manage its data.
         * Calls the "update" function of the script by default.
         */
        virtual void update();

        /**
         * Prepares a call to the given function.
         * Only one function can be prepared at once.
         */
        virtual void prepare(std::string const &name) = 0;

        /**
         * Pushes an integer argument for the function being prepared.
         */
        virtual void push(int) = 0;

        /**
         * Pushes a pointer argument to a game entity.
         * The interface can pass the pointer as an opaque value to the
         * scripting engine, if needed. This value will usually be passed
         * by the script to some callabck functions.
         */
        virtual void push(Thing *) = 0;

        /**
         * Executes the function being prepared.
         * @return the value returned by the script.
         */
        virtual int execute() = 0;

        /**
         * Sets associated map.
         */
        void setMap(MapComposite *m)
        { mMap = m; }

        /**
         * Gets associated map.
         */
        MapComposite *getMap() const
        { return mMap; }

    private:
        MapComposite *mMap;
};

#endif
