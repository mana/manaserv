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

#include "guild.hpp"

#include <algorithm>

GuildMember::GuildMember(std::string name) :
    mName(name),
    mPermissions(0)
{
}

std::string GuildMember::getName() const
{
    return mName;
}

void GuildMember::setPermission(int perm)
{
    mPermissions = perm;
}

int GuildMember::getPermissions() const
{
    return mPermissions;
}

Guild::Guild(const std::string &name) :
mName(name)
{
}

Guild::~Guild()
{
}

void Guild::addMember(const std::string &playerName)
{
    GuildMember *member = new GuildMember(playerName);
    mMembers.push_back(member);
    if (checkInvited(playerName))
    {
        mInvited.remove(playerName);
    }
}

void Guild::removeMember(const std::string &playerName)
{
    GuildMember *member = getMember(playerName);
    if (member)
        mMembers.remove(member);
}

bool Guild::checkLeader(const std::string &playerName)
{
    // check that guild member permissions is set to LEADER
    int leader = 0;
    GuildMember *member = getMember(playerName);
    if (member)
        leader = member->getPermissions();
    if (leader == GuildMember::LEADER)
        return true;
    return false;

}

bool Guild::checkInvited(const std::string &playerName)
{
    return std::find(mInvited.begin(), mInvited.end(), playerName) != mInvited.end();
}

void Guild::addInvited(const std::string &playerName)
{
    mInvited.push_back(playerName);
}

bool Guild::checkInGuild(const std::string &playerName)
{
    GuildMember *member = getMember(playerName);
    return member ? true : false;
}

GuildMember* Guild::getMember(const std::string &playerName)
{
    std::list<GuildMember*>::iterator itr = mMembers.begin(), itr_end = mMembers.end();
    while (itr != itr_end)
    {
        if ((*itr)->getName() == playerName)
        {
            return (*itr);
        }

        ++itr;
    }

    return NULL;
}

bool Guild::canInvite(const std::string &playerName)
{
    // Guild members with permissions above NONE can invite
    // Check that guild members permissions are not NONE
    GuildMember *member = getMember(playerName);
    if (member->getPermissions() > GuildMember::NONE)
        return true;
    return false;
}

int Guild::getUserPermissions(const std::string &playerName)
{
    GuildMember *member = getMember(playerName);
    return member->getPermissions();
}
