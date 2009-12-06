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
 */

#ifndef GAMESERVER_QUEST_HPP
#define GAMESERVER_QUEST_HPP

#include <string>

class Character;

struct QuestCallback
{
    void (*handler)(Character *, const std::string &name,
                    const std::string &value, void *data);
    void *data;
};

/**
 * Gets the value associated to a quest variable.
 * @return false if no value was in cache.
 */
bool getQuestVar(Character *, const std::string &name, std::string &value);

/**
 * Sets the value associated to a quest variable.
 */
void setQuestVar(Character *, const std::string &name,
                 const std::string &value);

/**
 * Starts the recovery of a variable and returns immediatly. The callback will
 * be called once the value has been recovered.
 */
void recoverQuestVar(Character *, const std::string &name,
                     const QuestCallback &);

/**
 * Called by the handler of the account server when a value is received.
 */
void recoveredQuestVar(int id, const std::string &name,
                       const std::string &value);

#endif
