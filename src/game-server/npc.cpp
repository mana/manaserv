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

#include "game-server/character.h"
#include "game-server/npc.h"
#include "scripting/script.h"

Script::Ref NPC::mStartCallback;
Script::Ref NPC::mNextCallback;
Script::Ref NPC::mChooseCallback;
Script::Ref NPC::mIntegerCallback;
Script::Ref NPC::mStringCallback;
Script::Ref NPC::mUpdateCallback;

NPC::NPC(const std::string &name, int id, Script *s):
    Being(OBJECT_NPC),
    mScript(s),
    mID(id),
    mEnabled(true)
{
    setName(name);
}

void NPC::enable(bool enabled)
{
    mEnabled = enabled;
}

void NPC::update()
{
    if (!mScript || !mEnabled || !mUpdateCallback.isValid())
        return;
    mScript->prepare(mUpdateCallback);
    mScript->push(this);
    mScript->execute();
}

void NPC::prompt(Character *ch, bool restart)
{
    if (!mScript || !mEnabled || !mStartCallback.isValid()
            || !mNextCallback.isValid())
        return;
    mScript->prepare(restart ? mStartCallback : mNextCallback);
    mScript->push(this);
    mScript->push(ch);
    mScript->execute();
}

void NPC::select(Character *ch, int v)
{
    if (!mScript || !mEnabled || !mChooseCallback.isValid())
        return;
    mScript->prepare(mChooseCallback);
    mScript->push(this);
    mScript->push(ch);
    mScript->push(v);
    mScript->execute();
}

void NPC::integerReceived(Character *ch, int v)
{
    if (!mScript || !mEnabled || !mIntegerCallback.isValid())
        return;
    mScript->prepare(mIntegerCallback);
    mScript->push(this);
    mScript->push(ch);
    mScript->push(v);
    mScript->execute();
}

void NPC::stringReceived(Character *ch, const std::string &v)
{
    if (!mScript || !mEnabled || !mStringCallback.isValid())
        return;
    mScript->prepare(mStringCallback);
    mScript->push(this);
    mScript->push(ch);
    mScript->push(v);
    mScript->execute();
}
