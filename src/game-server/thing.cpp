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

#include <cassert>

#include "game-server/thing.hpp"

#include "game-server/eventlistener.hpp"

Thing::~Thing()
{
    /* As another object will stop listening and call removeListener when it is
       deleted, the following assertion ensures that all the calls to
       removeListener have been performed will this object was still alive. It
       is not strictly necessary, as there are cases where no removal is
       performed (e.g. ~SpawnArea). But this is rather exceptional, so keep the
       assertion to catch all the other forgotten calls to removeListener. */
    assert(mListeners.empty());
}

void Thing::addListener(const EventListener *l)
{
    mListeners.insert(l);
}

void Thing::removeListener(const EventListener *l)
{
    mListeners.erase(l);
}

void Thing::inserted()
{
    for (Listeners::iterator i = mListeners.begin(),
         i_end = mListeners.end(); i != i_end;)
    {
        const EventListener &l = **i;
        ++i; // In case the listener removes itself from the list on the fly.
        if (l.dispatch->inserted) l.dispatch->inserted(&l, this);
    }
}

void Thing::removed()
{
    for (Listeners::iterator i = mListeners.begin(),
         i_end = mListeners.end(); i != i_end;)
    {
        const EventListener &l = **i;
        ++i; // In case the listener removes itself from the list on the fly.
        if (l.dispatch->removed) l.dispatch->removed(&l, this);
    }
}
