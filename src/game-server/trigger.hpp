/*
 *  The Mana Server
 *  Copyright (C) 2006-2010  The Mana World Development Team
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

#ifndef TRIGGER_HPP
#define TRIGGER_HPP

#include "point.h"
#include "game-server/thing.hpp"
#include "scripting/script.hpp"

class Actor;

class TriggerAction
{
    public:
        virtual ~TriggerAction() {}
        virtual void process(Actor *obj) = 0;
};

class WarpAction : public TriggerAction
{
    public:
        WarpAction(MapComposite *m, int x, int y)
          : mMap(m), mX(x), mY(y) {}

        virtual void process(Actor *obj);

    private:
        MapComposite *mMap;
        unsigned short mX, mY;
};

class ScriptAction : public TriggerAction
{
    public:
        ScriptAction(Script *script, const std::string &function, int arg)
          : mScript(script), mFunction(function), mArg(arg) {}

        virtual void process(Actor *obj);

    private:
        Script *mScript;        // Script object to be called
        std::string mFunction;  // Name of the function called in the script object
        int mArg;               // Argument passed to script function (meaning is function-specific)
};

class TriggerArea : public Thing
{
    public:
        /**
         * Creates a rectangular trigger for a given map.
         */
        TriggerArea(MapComposite *m, const Rectangle &r, TriggerAction *ptr, bool once)
          : Thing(OBJECT_OTHER, m), mZone(r), mAction(ptr), mOnce(once) {}

        virtual void update();

    private:
        Rectangle mZone;
        TriggerAction *mAction;
        bool mOnce;
        std::set<Actor *> mInside;
};

#endif
