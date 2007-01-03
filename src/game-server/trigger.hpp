/*
 *  The Mana World Server
 *  Copyright 2006 The Mana World Development Team
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

#ifndef _TMWSERV_TRIGGER
#define _TMWSERV_TRIGGER

#include "point.h"
#include "game-server/object.hpp"

class TriggerAction
{
    public:
        virtual ~TriggerAction() {}
        virtual void process(Object *) = 0;
};

class WarpAction: public TriggerAction
{
    public:
        WarpAction(int m, int x, int y)
          : mMap(m), mX(x), mY(y) {}

        virtual void process(Object *);

    private:
        unsigned short mMap, mX, mY;
};

class TriggerArea: public Thing
{
    public:
        /**
         * Creates a rectangular trigger for a given map.
         */
        TriggerArea(int, Rectangle const &, TriggerAction *);
        virtual void update();

    private:
        Rectangle mZone;
        TriggerAction *mAction;
};

#endif
