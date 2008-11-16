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
 */

#include "guild.hpp"

#include <algorithm>

Guild::Guild(const std::string &name) :
mName(name)
{
}

Guild::~Guild()
{
}

void Guild::addMember(int playerId, int permissions)
{
    // create new guild member
    GuildMember *member = new GuildMember();
    member->mId = playerId;
    member->mPermissions = permissions;

    // add new guild member to guild
    mMembers.push_back(member);

    if (checkInvited(playerId))
    {
        mInvited.remove(playerId);
    }
}

void Guild::removeMember(int playerId)
{
    GuildMember *member = getMember(playerId);
    if (member)
        mMembers.remove(member);
}

bool Guild::checkLeader(int playerId)
{
    int leader = 0;
    GuildMember *member = getMember(playerId);
    // check member exists
    if (member)
        leader = member->mPermissions;
    // check permissions
    if (leader == GuildMember::LEADER)
        return true;
    return false;

}

void Guild::setLeader(int playerId)
{
    GuildMember *member = getMember(playerId);
    if (member)
    {
        member->mPermissions = GuildMember::LEADER;
    }
}

bool Guild::checkInvited(int playerId)
{
    return std::find(mInvited.begin(), mInvited.end(), playerId) != mInvited.end();
}

void Guild::addInvited(int playerId)
{
    mInvited.push_back(playerId);
}

bool Guild::checkInGuild(int playerId)
{
    GuildMember *member = getMember(playerId);
    return member ? true : false;
}

GuildMember* Guild::getMember(int playerId)
{
    std::list<GuildMember*>::iterator itr = mMembers.begin(), itr_end = mMembers.end();
    while (itr != itr_end)
    {
        if ((*itr)->mId == playerId)
        {
            return (*itr);
        }

        ++itr;
    }

    return NULL;
}

bool Guild::canInvite(int playerId)
{
    // Guild members with permissions above NONE can invite
    // Check that guild members permissions are not NONE
    GuildMember *member = getMember(playerId);
    if (member->mPermissions > GuildMember::NONE)
        return true;
    return false;
}

int Guild::getUserPermissions(int playerId)
{
    GuildMember *member = getMember(playerId);
    return member->mPermissions;
}

void Guild::setUserPermissions(int playerId, int level)
{
    GuildMember *member = getMember(playerId);
    member->mPermissions = level;
}
