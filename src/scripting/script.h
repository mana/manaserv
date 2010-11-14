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

#ifndef SCRIPTING_SCRIPT_H
#define SCRIPTING_SCRIPT_H

#include <string>

#include "game-server/character.h"
#include "game-server/eventlistener.h"

class MapComposite;
class Thing;

/**
 * Abstract interface for calling functions written in an external language.
 */
class Script
{
    public:

        typedef Script *(*Factory)();

        /**
         * Registers a new scripting engine.
         */
        static void registerEngine(const std::string &, Factory);

        /**
         * Creates a new script context for a given engine.
         */
        static Script *create(const std::string &engine);

        /**
         * Constructor.
         */
        Script();

        /**
         * Destructor.
         */
        virtual ~Script() {}

        /**
         * Loads a chunk of text into script context and executes its global
         * statements.
         *
         * @param prog the program text to load
         * @param name the name of the text, used for error reporting
         */
        virtual void load(const char *prog, const char *name) = 0;

        /**
         * Loads a text file into script context and executes its global
         * statements.
         */
        virtual bool loadFile(const std::string &);

        /**
         * Loads a chunk of text and considers it as an NPC handler. This
         * handler will later be used to create the given NPC.
         */
        virtual void loadNPC(const std::string &name, int id, int x, int y,
                             const char *);

        /**
         * Called every tick for the script to manage its data.
         * Calls the "update" function of the script by default.
         */
        virtual void update();

        /**
         * Prepares a call to the given function.
         * Only one function can be prepared at once.
         */
        virtual void prepare(const std::string &name) = 0;

        /**
         * Pushes an integer argument for the function being prepared.
         */
        virtual void push(int) = 0;

        /**
         * Pushes a string argument for the function being prepared.
         */
        virtual void push(const std::string &) = 0;

        /**
         * Pushes a pointer argument to a game entity.
         * The interface can pass the pointer as an opaque value to the
         * scripting engine, if needed. This value will usually be passed
         * by the script to some callback functions.
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

        EventListener *getScriptListener()
        { return &mEventListener; }

        virtual void processDeathEvent(Being *thing) = 0;

        virtual void processRemoveEvent(Thing *thing) = 0;

        /**
         * Runs a function from the global event script file
         */
        static bool executeGlobalEventFunction(const std::string &function, Being *obj);
        static void addDataToSpecial(int specialId, Special *special);
        static bool performSpecialAction(int specialId, Being *caster);


    protected:
        static Script *globalEventScript;
        static Script *specialActionsScript;
        std::string mScriptFile;

    private:
        MapComposite *mMap;
        EventListener mEventListener; /**< Tracking of being deaths. */

    friend struct ScriptEventDispatch;
};

struct ScriptEventDispatch: EventDispatch
{
    ScriptEventDispatch()
    {
        typedef EventListenerFactory< Script, &Script::mEventListener > Factory;
        died = &Factory::create< Being, &Script::processDeathEvent >::function;
        removed = &Factory::create< Thing, &Script::processRemoveEvent >::function;
    }
};

static ScriptEventDispatch scriptEventDispatch;

#endif
