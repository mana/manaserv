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

#include <algorithm>
#include <list>
#include <map>
#include <string>

#include "game-server/quest.hpp"

#include "defines.h"
#include "game-server/accountconnection.hpp"
#include "game-server/character.hpp"
#include "game-server/deathlistener.hpp"
#include "utils/logger.h"

typedef std::list< QuestCallback > QuestCallbacks;
typedef std::map< std::string, QuestCallbacks > PendingVariables;

struct PendingQuest
{
    Character *character;
    PendingVariables variables;
};

typedef std::map< int, PendingQuest > PendingQuests;

static PendingQuests pendingQuests;

bool getQuestVar(Character *ch, std::string const &name, std::string &value)
{
    std::map< std::string, std::string >::iterator
        i = ch->questCache.find(name);
    if (i == ch->questCache.end()) return false;
    value = i->second;
    return true;
}

void setQuestVar(Character *ch, std::string const &name,
                 std::string const &value)
{
    std::map< std::string, std::string >::iterator
        i = ch->questCache.lower_bound(name);
    if (i == ch->questCache.end() || i->first != name)
    {
        ch->questCache.insert(i, std::make_pair(name, value));
    }
    else if (i->second == value)
    {
        return;
    }
    else
    {
        i->second = value;
    }
    accountHandler->updateQuestVar(ch, name, value);
}

/**
 * Listener for deleting related quests when a character disappears.
 */
struct QuestDeathListener: DeathListener
{
    void deleted(Being *b)
    {
        /* FIXME: At this point, Character has already been destroyed and we
           are potentially reading garbage or segfaulting. This is a misfeature
           of DeathListener and it should be fixed there. Anyway, as we are
           calling a non-virtual method and it accesses a primitive datatype,
           we should be safe with any compiler without vicious compliance. */
        int id = static_cast< Character * >(b)->getDatabaseID();
        pendingQuests.erase(id);
    }
};

static QuestDeathListener questDeathListener;

void recoverQuestVar(Character *ch, std::string const &name,
                     QuestCallback const &f)
{
    int id = ch->getDatabaseID();
    PendingQuests::iterator i = pendingQuests.lower_bound(id);
    if (i == pendingQuests.end() || i->first != id)
    {
        i = pendingQuests.insert(i, std::make_pair(id, PendingQuest()));
        i->second.character = ch;
        /* Register a listener, because we cannot afford to get invalid
           pointers, when we finally recover the variable. */
        ch->addDeathListener(&questDeathListener);
    }
    i->second.variables[name].push_back(f);
    accountHandler->requestQuestVar(ch, name);
}

void recoveredQuestVar(int id, std::string const &name,
                       std::string const &value)
{
    PendingQuests::iterator i = pendingQuests.find(id);
    if (i == pendingQuests.end()) return;
    PendingVariables &variables = i->second.variables;
    PendingVariables::iterator j = variables.find(name);
    if (j == variables.end())
    {
        LOG_ERROR("Account server recovered an unexpected quest variable.");
        return;
    }

    Character *ch = i->second.character;
    ch->questCache[name] = value;

    // Call the registered callbacks.
    for (QuestCallbacks::const_iterator k = j->second.begin(),
         k_end = j->second.end(); k != k_end; ++k)
    {
        k->handler(ch, name, value, k->data);
    }

    variables.erase(j);
    if (variables.empty())
    {
        pendingQuests.erase(i);
    }
}
