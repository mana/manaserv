/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
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

#include "game-server/statuseffect.h"

#include "scripting/script.h"
#include "game-server/being.h"

StatusEffect::StatusEffect(int id):
    mId(id),
    mScript(0)
{
}

StatusEffect::~StatusEffect()
{
    delete mScript;
}

void StatusEffect::tick(Being *target, int count)
{
    if (mScript)
    {
        mScript->setMap(target->getMap());
        mScript->prepare("tick");
        mScript->push(target);
        mScript->push(count);
        mScript->execute();
    }
}
