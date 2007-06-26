/*
 *  guildmanager.cpp
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

#include "guildmanager.hpp"

#include "account-server/characterdata.hpp"
#include "account-server/guild.hpp"
#include "account-server/storage.hpp"

GuildManager::GuildManager()
{
    // Load stored guilds from db
    Storage &store = Storage::instance("tmw");
    mGuilds = store.getGuildList();
}

GuildManager::~GuildManager()
{
    for(std::list<Guild*>::iterator itr = mGuilds.begin(); itr != mGuilds.end(); ++itr)
    {
        Guild *guild = (*itr);
        delete guild;
    }
}

short GuildManager::createGuild(const std::string &name, CharacterData* player)
{
    Guild *guild = new Guild(name);
    // Add guild to db
    Storage &store = Storage::instance("tmw");
    store.addGuild(guild);
    
    // Make sure to add guild to mGuilds before searching for it
    // to add the player
    mGuilds.push_back(guild);
    addGuildMember(guild->getId(), player);
    
    return guild->getId();
}

void GuildManager::removeGuild(short guildId)
{
    Guild *guild = findById(guildId);
    if(!guild)
        return;
    Storage &store = Storage::instance("tmw");
    store.removeGuild(guild);
}

void GuildManager::addGuildMember(short guildId, CharacterData *player)
{
    Guild *guild = findById(guildId);
    if(!guild)
        return;
    Storage &store = Storage::instance("tmw");
    store.addGuildMember(guildId, player->getName());
    guild->addMember(player);
}

void GuildManager::removeGuildMember(short guildId, CharacterData *player)
{
    Guild *guild = findById(guildId);
    if(!guild)
        return;
    Storage &store = Storage::instance("tmw");
    store.removeGuildMember(guildId, player->getName());
    guild->removeMember(player);
    if(guild->totalMembers() == 0)
    {
        removeGuild(guildId);
    }
}

Guild *GuildManager::findById(short id)
{
    for(std::list<Guild*>::iterator itr = mGuilds.begin(); itr != mGuilds.end(); ++itr)
    {
        Guild *guild = (*itr);
        if(guild->getId() == id)
        {
            return guild;
        }
    }
    return NULL;
}

Guild *GuildManager::findByName(const std::string &name)
{
    std::list<Guild*>::iterator itr = mGuilds.begin();
    for(; itr != mGuilds.end(); ++itr)
    {
        Guild *guild = (*itr);
        if(guild->getName() == name)
        {
            return guild;
        }
    }
    return NULL;
}

bool GuildManager::doesExist(const std::string &name)
{
    Guild *guild = findByName(name);
    if(guild)
    {
        return true;
    }
    return false;
}
