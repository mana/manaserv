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
 *  $Id: guild.hpp 3549 2007-08-30 16:20:33Z gmelquio $
 */

#ifndef _TMWSERV_CHATSERVER_GUILD_H_
#define _TMWSERV_CHATSERVER_GUILD_H_

#include <string>
#include <list>

/**
 * A guild and its members.
 */
class Guild
{
    public:
        typedef std::list<std::string> guildMembers;

        /**
         * Constructor.
         */
        Guild(const std::string &name);

        /**
         * Destructor.
         */
        ~Guild();

        /**
         * Add a member to the guild.
         */
        void addMember(const std::string &playerName);

        /**
         * Remove a member from the guild.
         */
        void removeMember(const std::string &playerName);

        /**
         * Check player is the leader of the guild.
         */
        bool checkLeader(const std::string &playerName);

        /**
         * Set the ID of the guild.
         */
        void setId(int id)
        { mId = id; }

        /**
         * Check if player has been invited to the guild.
         */
        bool checkInvited(const std::string &playerName);

        /**
         * Add a player to the invite list.
         */
        void addInvited(const std::string &playerName);

        /**
         * Returns the name of the guild.
         */
        const std::string& getName() const
        { return mName; }

        /**
         * Returns the ID of the guild.
         */
        int getId() const
        { return mId; }

        /**
         * Returns the total number of members in the guild.
         */
        int totalMembers() const
        { return mMembers.size(); }

        /**
         * Get a member in the guild.
         */
        const std::string& getMember(int i) const;

        /**
         * Find member by name.
         */
        bool checkInGuild(const std::string &playerName);

    private:
        short mId;
        std::string mName;
        std::list<std::string> mMembers;
        std::list<std::string> mInvited;
};

#endif
