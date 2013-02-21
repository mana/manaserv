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

#include "game-server/trigger.h"

#include "game-server/character.h"
#include "game-server/mapcomposite.h"
#include "game-server/actor.h"
#include "game-server/state.h"

#include "utils/logger.h"

#include <cassert>

void WarpAction::process(Actor *obj)
{
    if (obj->getType() == OBJECT_CHARACTER)
    {
        GameState::enqueueWarp(static_cast< Character * >(obj), mMap, mX, mY);
    }
}

ScriptAction::ScriptAction(Script *script, Script::Ref callback, int arg) :
    mScript(script),
    mCallback(callback),
    mArg(arg)
{
    assert(mCallback.isValid());
}

void ScriptAction::process(Actor *obj)
{
    LOG_DEBUG("Script trigger area activated: "
              << "(" << obj << ", " << mArg << ")");

    mScript->prepare(mCallback);
    mScript->push(obj);
    mScript->push(mArg);
    mScript->execute(obj->getMap());
}

void TriggerArea::update()
{
    std::set<Actor*> insideNow;
    for (BeingIterator i(getMap()->getInsideRectangleIterator(mZone)); i; ++i)
    {
        // Don't deal with unitialized actors.
        if (!(*i) || !(*i)->isPublicIdValid())
            continue;

        // The BeingIterator returns the mapZones in touch with the rectangle
        // area. On the other hand, the beings contained in the map zones
        // may not be within the rectangle area. Hence, this additional
        // contains() condition.
        if (mZone.contains((*i)->getPosition()))
        {
            insideNow.insert(*i);

            if (!mOnce || mInside.find(*i) == mInside.end())
            {
                mAction->process(*i);
            }
        }
    }
    mInside.swap(insideNow); //swapping is faster than assigning
}
