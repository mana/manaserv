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

#include "game-server/character.hpp"
#include "game-server/npc.hpp"
#include "scripting/script.hpp"

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
    if (!mScript || !mEnabled) return;
    mScript->prepare("npc_update");
    mScript->push(this);
    mScript->execute();
}

void NPC::prompt(Character *ch, bool restart)
{
    if (!mScript || !mEnabled) return;
    mScript->prepare(restart ? "npc_start" : "npc_next");
    mScript->push(this);
    mScript->push(ch);
    mScript->execute();
}

void NPC::select(Character *ch, int v)
{
    if (!mScript || !mEnabled) return;
    mScript->prepare("npc_choose");
    mScript->push(this);
    mScript->push(ch);
    mScript->push(v);
    mScript->execute();
}

void NPC::integerReceived(Character *ch, int v)
{
    if (!mScript || !mEnabled) return;
    mScript->prepare("npc_integer");
    mScript->push(this);
    mScript->push(ch);
    mScript->push(v);
    mScript->execute();
}

void NPC::stringReceived(Character *ch, const std::string &v)
{
    if (!mScript || !mEnabled) return;
    mScript->prepare("npc_string");
    mScript->push(this);
    mScript->push(ch);
    mScript->push(v);
    mScript->execute();
}
