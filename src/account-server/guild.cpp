/*
 *  guild.cpp
 *  A file part of The Mana World
 *
 *  Created by David Athay on 01/03/2007.
 *  
 * Copyright (c) 2007, The Mana World Development Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * My name may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 * $Id$
 */

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

bool Guild::checkLeader(CharacterData* player)
{
    CharacterData* leader = mMembers.front();
    if(leader == player)
        return true;
    return false;
}

bool Guild::checkInvited(const std::string &name)
{
    return (std::find(mInvited.begin(), mInvited.end(), name) != mInvited.end());
}

void Guild::addInvited(const std::string &name)
{
    mInvited.push_back(name);
}

const std::string& Guild::getName() const
{
    return mName;
}

std::string Guild::getMember(int i) const
{
    int x = 0;
    for(guildMembers::const_iterator itr = mMembers.begin();
        itr != mMembers.end();
        ++itr, ++x)
    {
        if(x == i)
        {
            CharacterData *player = (*itr);
            return player->getName();
        }
    }
    return "";
}

bool Guild::checkInGuild(const std::string &name)
{
    for(guildMembers::iterator itr = mMembers.begin(); itr != mMembers.end(); ++itr)
    {
        CharacterData *player = (*itr);
        if(player->getName() == name)
        {
            return true;
        }
    }
    return false;
}
