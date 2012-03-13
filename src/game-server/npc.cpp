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
#include "game-server/gamehandler.h"
#include "game-server/map.h"
#include "game-server/npc.h"
#include "net/messageout.h"
#include "scripting/script.h"
#include "scripting/scriptmanager.h"

NPC::NPC(const std::string &name, int id):
    Being(OBJECT_NPC),
    mID(id),
    mEnabled(true)
{
    setWalkMask(Map::BLOCKMASK_WALL | Map::BLOCKMASK_MONSTER |
                Map::BLOCKMASK_CHARACTER);
    setName(name);
}

NPC::~NPC()
{
    Script *script = ScriptManager::currentState();
    script->unref(mTalkCallback);
    script->unref(mUpdateCallback);
}

void NPC::setEnabled(bool enabled)
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
    if (!mEnabled || !mTalkCallback.isValid())
        return;

    Script *script = ScriptManager::currentState();

    if (restart)
    {
        Script::Thread *thread = script->newThread();
        thread->mMap = getMap();
        script->prepare(mTalkCallback);
        script->push(this);
        script->push(ch);
        ch->startNpcThread(thread, getPublicID());
    }
    else
    {
        Script::Thread *thread = ch->getNpcThread();
        if (!thread || thread->mState != Script::ThreadPaused)
            return;

        script->prepareResume(thread);
        ch->resumeNpcThread();
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
    ch->resumeNpcThread();
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
    ch->resumeNpcThread();
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
    ch->resumeNpcThread();
}

void NPC::setTalkCallback(Script::Ref function)
{
    ScriptManager::currentState()->unref(mTalkCallback);
    mTalkCallback = function;
}

void NPC::setUpdateCallback(Script::Ref function)
{
    ScriptManager::currentState()->unref(mUpdateCallback);
    mUpdateCallback = function;
}
