/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#ifndef _TMWSERV_GAMESERVER_QUEST_
#define _TMWSERV_GAMESERVER_QUEST_

#include <string>

class Character;

struct QuestCallback
{
    void (*handler)(Character *, std::string const &name,
                    std::string const &value, void *data);
    void *data;
};

/**
 * Gets the value associated to a quest variable.
 * @return false if no value was in cache.
 */
bool getQuestVar(Character *, std::string const &name, std::string &value);

/**
 * Sets the value associated to a quest variable.
 */
void setQuestVar(Character *, std::string const &name,
                 std::string const &value);

/**
 * Starts the recovery of a variable and returns immediatly. The callback will
 * be called once the value has been recovered.
 */
void recoverQuestVar(Character *, std::string const &name,
                     QuestCallback const &);

/**
 * Called by the handler of the account server when a value is received.
 */
void recoveredQuestVar(int id, std::string const &name,
                       std::string const &value);

#endif
