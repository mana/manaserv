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

#ifndef _TMWSERV_ACCOUNTSERVER_GUILD_H_
#define _TMWSERV_ACCOUNTSERVER_GUILD_H_

#include <string>
#include <list>

class CharacterData;

class Guild
{
public:
    typedef std::list<CharacterData*> guildMembers;
    Guild(const std::string &name);
    ~Guild();
    
    /**
     * Add a member to the guild. 
     */
    void addMember(CharacterData* player);
    
    /**
     * Remove a member from the guild. 
     */
    void removeMember(CharacterData* player);
    
    /**
     * Check player is the leader of the guild. 
     */
    bool checkLeader(CharacterData* player);
    
    /**
     * Set the ID of the guild.
     */
    void setId(short id)
    {
        mId = id;
    }
    
    /**
     * Check if player has been invited to the guild. 
     */
    bool checkInvited(const std::string &name);
    
    /**
     * Add a player to the invite list.
     */
    void addInvited(const std::string &name);
    
    /**
     * Returns the name of the guild.
     */
    const std::string& getName() const;
    
    /**
     * Returns the ID of the guild.
     */
    short getId() const
    {
        return mId;
    }
    
    /**
     * Returns the total number of members in the guild.
     */
    short totalMembers() const
    {
        return mMembers.size();
    }
    
    /**
     * Get a member in the guild
     */
    std::string getMember(int i) const;
    
    /**
     * Find member by name
     */
    bool checkInGuild(const std::string &name);
    
private:
    short mId;
    std::string mName;
    std::list<CharacterData*> mMembers;
    std::list<std::string> mInvited;
};

#endif
