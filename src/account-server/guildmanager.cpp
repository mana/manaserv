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
    for (std::list<Guild*>::iterator itr = mGuilds.begin();
            itr != mGuilds.end(); ++itr)
    {
        delete *itr;
    }
}

short GuildManager::createGuild(const std::string &name, CharacterData *player)
{
    Guild *guild = new Guild(name);
    // Add guild to db
    Storage &store = Storage::instance("tmw");
    store.addGuild(guild);

    // Make sure to add guild to mGuilds before searching for it to add the
    // player
    mGuilds.push_back(guild);
    addGuildMember(guild->getId(), player);

    return guild->getId();
}

void GuildManager::removeGuild(short guildId)
{
    Guild *guild = findById(guildId);
    if (!guild)
        return;
    Storage &store = Storage::instance("tmw");
    store.removeGuild(guild);
}

void GuildManager::addGuildMember(short guildId, CharacterData *player)
{
    Guild *guild = findById(guildId);
    if (!guild)
        return;
    Storage &store = Storage::instance("tmw");
    store.addGuildMember(guildId, player->getName());
    guild->addMember(player);
}

void GuildManager::removeGuildMember(short guildId, CharacterData *player)
{
    Guild *guild = findById(guildId);
    if (!guild)
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
    for (std::list<Guild*>::iterator itr = mGuilds.begin();
            itr != mGuilds.end(); ++itr)
    {
        Guild *guild = (*itr);
        if (guild->getId() == id)
        {
            return guild;
        }
    }
    return NULL;
}

Guild *GuildManager::findByName(const std::string &name)
{
    for (std::list<Guild*>::iterator itr = mGuilds.begin();
            itr != mGuilds.end(); ++itr)
    {
        Guild *guild = (*itr);
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
