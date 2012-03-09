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
#include "scripting/scriptmanager.h"

Script::Ref NPC::mStartCallback;
Script::Ref NPC::mUpdateCallback;

NPC::NPC(const std::string &name, int id):
    Being(OBJECT_NPC),
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
    if (!mEnabled || !mUpdateCallback.isValid())
        return;

    Script *script = ScriptManager::currentState();
    script->prepare(mUpdateCallback);
    script->push(this);
    script->execute();
}

void NPC::prompt(Character *ch, bool restart)
{
    if (!mEnabled || !mStartCallback.isValid())
        return;

    Script *script = ScriptManager::currentState();

    if (restart)
    {
        Script::Thread *thread = script->newThread();
        thread->mMap = getMap();
        script->prepare(mStartCallback);
        script->push(this);
        script->push(ch);

        if (!script->resume())
            ch->setNpcThread(thread);
    }
    else
    {
        Script::Thread *thread = ch->getNpcThread();
        if (!thread || thread->mState != Script::ThreadPaused)
            return;

        script->prepareResume(thread);
        if (script->resume())
            ch->setNpcThread(0);
    }
}

void NPC::select(Character *ch, int index)
{
    if (!mEnabled)
        return;

    Script::Thread *thread = ch->getNpcThread();
    if (!thread || thread->mState != Script::ThreadExpectingNumber)
        return;

    Script *script = ScriptManager::currentState();
    script->prepareResume(thread);
    script->push(index);
    if (script->resume())
        ch->setNpcThread(0);
}

void NPC::integerReceived(Character *ch, int value)
{
    if (!mEnabled)
        return;

    Script::Thread *thread = ch->getNpcThread();
    if (!thread || thread->mState != Script::ThreadExpectingNumber)
        return;

    Script *script = ScriptManager::currentState();
    script->prepareResume(thread);
    script->push(value);
    if (script->resume())
        ch->setNpcThread(0);
}

void NPC::stringReceived(Character *ch, const std::string &value)
{
    if (!mEnabled)
        return;

    Script::Thread *thread = ch->getNpcThread();
    if (!thread || thread->mState != Script::ThreadExpectingString)
        return;

    Script *script = ScriptManager::currentState();
    script->prepareResume(thread);
    script->push(value);
    if (script->resume())
        ch->setNpcThread(0);
}
