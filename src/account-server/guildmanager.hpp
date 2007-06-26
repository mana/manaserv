/*
 *  guildmanager.hpp
 *  A file part of The Mana World
 *
 *  Created by David Athay on 05/03/2007.
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

#include <list>

class Guild;
class CharacterData;

class GuildManager
{
public:
    /*
     * Constructor/Destructor
     */
    GuildManager();
    ~GuildManager();
    
    /*
     * Create/Remove guild
     */
    short createGuild(const std::string &name, CharacterData *player);
    void removeGuild(short guildId);
    
    /*
     * Add member to guild
     */
    void addGuildMember(short guildId, CharacterData *player);
    
    /*
     * Remove member from guild
     */
    void removeGuildMember(short guildId, CharacterData *player);
    
    /*
     * Search for guilds
     */
    Guild *findById(short id);
    Guild *findByName(const std::string &name);
    
    /*
     * Check if guild exists
     */
    bool doesExist(const std::string &name);
    
private:
    std::list<Guild*> mGuilds;
};

extern GuildManager *guildManager;
