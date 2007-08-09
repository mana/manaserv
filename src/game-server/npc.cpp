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
 *
 *  $Id$
 */

#include "game-server/character.hpp"
#include "game-server/npc.hpp"
#include "scripting/script.hpp"

NPC::NPC(int id, Script *s):
    Being(OBJECT_NPC, 65535), mScript(s), mID(id)
{
}

void NPC::update()
{
    if (!mScript) return;
    mScript->prepare("npc_update");
    mScript->push(this);
    mScript->execute();
}

void NPC::prompt(Character *ch, bool restart)
{
    if (!mScript) return;
    mScript->prepare(restart ? "npc_start" : "npc_next");
    mScript->push(this);
    mScript->push(ch);
    mScript->execute();
}

void NPC::select(Character *ch, int v)
{
    if (!mScript) return;
    mScript->prepare("npc_choose");
    mScript->push(this);
    mScript->push(ch);
    mScript->push(v);
    mScript->execute();
}

