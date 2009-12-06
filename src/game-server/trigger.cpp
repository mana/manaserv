/*
 *  The Mana Server
 *  Copyright (C) 2006  The Mana World Development Team
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

#include "game-server/trigger.hpp"

#include "game-server/character.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/actor.hpp"
#include "game-server/state.hpp"

#include "utils/logger.h"

void WarpAction::process(Actor *obj)
{
    if (obj->getType() == OBJECT_CHARACTER)
    {
        GameState::enqueueWarp(static_cast< Character * >(obj), mMap, mX, mY);
    }
}

void ScriptAction::process(Actor *obj)
{
    LOG_DEBUG("Script trigger area activated: " << mFunction
              << "(" << obj << ", " << mArg << ")");
    if (!mScript || mFunction.empty())
        return;
    mScript->prepare(mFunction);
    mScript->push(obj);
    mScript->push(mArg);
    mScript->execute();
}

void TriggerArea::update()
{
    std::set<Actor*> insideNow;
    for (BeingIterator i(getMap()->getInsideRectangleIterator(mZone)); i; ++i)
    {
        if (mZone.contains((*i)->getPosition())) //<-- Why is this additional condition necessary? Shouldn't getInsideRectangleIterator already exclude those outside of the zone? --Crush
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
