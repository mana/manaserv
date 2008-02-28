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
 *  $Id: guild.cpp 3549 2007-08-30 16:20:33Z gmelquio $
 */
#include "guild.hpp"


Guild::Guild(const std::string &name) :
mName(name)
{
}

Guild::~Guild()
{
}

void Guild::addMember(const std::string &playerName)
{
    mMembers.push_back(playerName);
}

void Guild::removeMember(const std::string &playerName)
{
    mMembers.remove(playerName);
}

bool Guild::checkLeader(const std::string &playerName)
{
    std::string leaderName = mMembers.front();
    return leaderName == playerName;
}

bool Guild::checkInvited(const std::string &playerName)
{
    return std::find(mInvited.begin(), mInvited.end(), playerName) != mInvited.end();
}

void Guild::addInvited(const std::string &playerName)
{
    mInvited.push_back(playerName);
}

const std::string& Guild::getMember(int i) const
{
    int x = 0;
    for (guildMembers::const_iterator itr = mMembers.begin();
        itr != mMembers.end();
        ++itr, ++x)
    {
        if (x == i)
        {
            return (*itr);
        }
    }
    return NULL;
}

bool Guild::checkInGuild(const std::string &playerName)
{
    return std::find(mMembers.begin(), mMembers.end(), playerName) != mMembers.end();
}
