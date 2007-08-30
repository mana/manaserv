/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
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

#if 0
#include "guild.hpp"

#include "account-server/characterdata.hpp"
#include "account-server/storage.hpp"

Guild::Guild(const std::string &name) :
mName(name)
{
}

Guild::~Guild()
{
}

void Guild::addMember(CharacterData* player)
{
    mMembers.push_back(player);
}

void Guild::removeMember(CharacterData* player)
{
    mMembers.remove(player);
}

bool Guild::checkLeader(CharacterData *player)
{
    CharacterData *leader = mMembers.front();
    return leader == player;
}

bool Guild::checkInvited(const std::string &name)
{
    return std::find(mInvited.begin(), mInvited.end(), name) != mInvited.end();
}

void Guild::addInvited(const std::string &name)
{
    mInvited.push_back(name);
}

std::string Guild::getMember(int i) const
{
    int x = 0;
    for (guildMembers::const_iterator itr = mMembers.begin();
        itr != mMembers.end();
        ++itr, ++x)
    {
        if (x == i)
        {
            CharacterData *player = (*itr);
            return player->getName();
        }
    }
    return "";
}

bool Guild::checkInGuild(const std::string &name)
{
    for (guildMembers::iterator itr = mMembers.begin(); itr != mMembers.end(); ++itr)
    {
        CharacterData *player = (*itr);
        if (player->getName() == name)
        {
            return true;
        }
    }
    return false;
}
#endif
