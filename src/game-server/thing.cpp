/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

void Thing::addListener(EventListener const *l)
{
    mListeners.insert(l);
}

void Thing::removeListener(EventListener const *l)
{
    mListeners.erase(l);
}

void Thing::inserted()
{
    for (Listeners::iterator i = mListeners.begin(),
         i_end = mListeners.end(); i != i_end;)
    {
        EventListener const &l = **i;
        ++i; // In case the listener removes itself from the list on the fly.
        if (l.dispatch->inserted) l.dispatch->inserted(&l, this);
    }
}

void Thing::removed()
{
    for (Listeners::iterator i = mListeners.begin(),
         i_end = mListeners.end(); i != i_end;)
    {
        EventListener const &l = **i;
        ++i; // In case the listener removes itself from the list on the fly.
        if (l.dispatch->removed) l.dispatch->removed(&l, this);
    }
}
