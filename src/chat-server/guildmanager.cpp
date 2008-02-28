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
 *  $Id: guildmanager.cpp 3549 2007-08-30 16:20:33Z gmelquio $
 */

#include "guildmanager.hpp"
#include "guild.hpp"
#include "account-server/dalstorage.hpp"

GuildManager::GuildManager()
{
    // Load stored guilds from db
    mGuilds = storage->getGuildList();
}

GuildManager::~GuildManager()
{
    for (std::list<Guild*>::iterator itr = mGuilds.begin();
            itr != mGuilds.end(); ++itr)
    {
        delete *itr;
    }
    mGuilds.clear();
}

short GuildManager::createGuild(const std::string &name, const std::string &playerName)
{
    Guild *guild = new Guild(name);
    // Add guild to db
    storage->addGuild(guild);

    // Make sure to add guild to mGuilds before searching for it to add the
    // player
    mGuilds.push_back(guild);
    addGuildMember(guild->getId(), playerName);

    return guild->getId();
}

void GuildManager::removeGuild(short guildId)
{
    Guild *guild = findById(guildId);
    if (!guild)
        return;
    storage->removeGuild(guild);
    mGuilds.remove(guild);
    delete guild;
}

void GuildManager::addGuildMember(short guildId, const std::string &playerName)
{
    Guild *guild = findById(guildId);
    if (!guild)
        return;
    storage->addGuildMember(guildId, playerName);
    guild->addMember(playerName);
}

void GuildManager::removeGuildMember(short guildId, const std::string &playerName)
{
    Guild *guild = findById(guildId);
    if (!guild)
        return;
    storage->removeGuildMember(guildId, playerName);
    guild->removeMember(playerName);
    if(guild->totalMembers() == 0)
    {
        removeGuild(guildId);
    }
}

Guild *GuildManager::findById(short id)
{
    Guild *guild;
    for (std::list<Guild*>::iterator itr = mGuilds.begin(),
            itr_end = mGuilds.end();
            itr != itr_end; ++itr)
    {
        guild = (*itr);
        if (guild->getId() == id)
        {
            return guild;
        }
    }
    return NULL;
}

Guild *GuildManager::findByName(const std::string &name)
{
    Guild *guild;
    for (std::list<Guild*>::iterator itr = mGuilds.begin(),
            itr_end = mGuilds.end();
            itr != itr_end; ++itr)
    {
        guild = (*itr);
        if (guild->getName() == name)
        {
            return guild;
        }
    }
    return NULL;
}

bool GuildManager::doesExist(const std::string &name)
{
    return findByName(name) != NULL;
}
