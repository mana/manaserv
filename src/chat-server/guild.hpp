/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CHATSERVER_GUILD_H
#define CHATSERVER_GUILD_H

#include <string>
#include <list>

/**
 * Guild members
 */

struct GuildMember
{
public:
    int mId;
    std::string mName;
    int mPermissions;

};

/**
 * A guild and its members.
 */
class Guild
{
    public:
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
         * Removes a user from invite list if on it
         */
        void addMember(int playerId, int permissions = 0);

        /**
         * Remove a member from the guild.
         */
        void removeMember(int playerId);

        /**
         * Return owner id
         */
        int getOwner();

        /**
         * Set player as owner of the guild.
         */
        void setOwner(int playerId);

        /**
         * Set the ID of the guild.
         */
        void setId(int id)
        { mId = id; }

        /**
         * Check if player has been invited to the guild.
         */
        bool checkInvited(int playerId);

        /**
         * Add a player to the invite list.
         */
        void addInvited(int playerId);

        /**
         * Returns the name of the guild.
         */
        std::string getName() const
        { return mName; }

        /**
         * Returns the ID of the guild.
         */
        int getId() const
        { return mId; }

        /**
         * Returns a list of the members in this guild.
         */
        std::list<GuildMember*> getMembers() const
        { return mMembers; }

        /**
         * Returns the number of members in the guild.
         */
        int memberCount() const
        { return mMembers.size(); }

        /**
         * Find member by name.
         */
        bool checkInGuild(int playerId);

        /**
         * Returns whether a user can invite
         */
        bool canInvite(int playerId);

        /**
         * Returns a users permissions
         */
        int getUserPermissions(int playerId);

        /**
         * Sets a users permissions
         */
        void setUserPermissions(int playerId, int level);

    protected:
        /**
         * Return a member based on their character name
         */
        GuildMember* getMember(int playerId);

    private:
        short mId;
        std::string mName;
        std::list<GuildMember*> mMembers;
        std::list<int> mInvited;
};

#endif
