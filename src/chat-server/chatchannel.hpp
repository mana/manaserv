/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#ifndef _TMWSERV_CHATCHANNEL_H_
#define _TMWSERV_CHATCHANNEL_H_

#include <string>
#include <vector>

/**
 * A chat channel. Optionally a channel is private, in which case a password is
 * required to join it.
 *
 * No logic is currently associated with a chat channel except for making sure
 * that no user joins the channel twice and checking that a user who leaves
 * actually existed in the channel.
 *
 * @todo <b>b_lindeijer:</b> It would be nicer when some more logic could be
 *       placed in this class to remove some weight from the ChatHandler.
 *       Referencing ChatClient instances would also be nicer than to store
 *       only the names of the characters.
 */
class ChatChannel
{
    public:
        typedef std::vector<std::string> ChannelUsers;

        /**
         * Constructor.
         *
         * @todo <b>b_lindeijer:</b> I would say a channel can be defined as
         *       private when a non-empty password is set, in which case we can
         *       get rid of the privacy parameter.
         *
         * @param name         the name of the channel.
         * @param announcement a welcome message.
         * @param password     password (for private channels).
         * @param privacy      whether this channel is private.
         */
        ChatChannel(short id,
                    const std::string &name,
                    const std::string &announcement = "",
                    const std::string &password = "",
                    bool privacy = false);

        /**
         * Get the ID of the channel.
         */
        short getId() const
        { return mId; }

        /**
         * Get the name of the channel.
         */
        const std::string& getName() const
        { return mName; }

        /**
         * Get the announcement string of the channel.
         */
        const std::string& getAnnouncement() const
        { return mAnnouncement; }

        /**
         * Get the password of the channel.
         */
        const std::string& getPassword() const
        { return mPassword; }

        /**
         * Returns whether this channel is private.
         */
        bool isPrivate() const
        { return mPrivate; }

        /**
         * Sets the name of the channel.
         */
        void setName(const std::string &channelName);

        /**
         * Sets the announcement string of the channel.
         */
        void setAnnouncement(const std::string &channelAnnouncement);

        /**
         * Sets the password of the channel.
         */
        void setPassword(const std::string &channelPassword);

        /**
         * Gets the list of the users registered in the channel.
         */
        const ChannelUsers& getUserList() const
        { return mRegisteredUsers; }

        /**
         * Adds a user to the channel.
         *
         * @return whether the user was successfully added
         */
        bool addUser(const std::string &name);

        /**
         * Removes a user from the channel.
         *
         * @return whether the user was successfully removed
         */
        bool removeUser(const std::string &name);

        /**
         * Empties a channel from its users (admin included).
         */
        void removeAllUsers();

    private:
        short mId;                     /**< The ID of the channel. */
        std::string mName;             /**< The name of the channel. */
        std::string mAnnouncement;     /**< Welcome message. */
        std::string mPassword;         /**< The channel password. */
        ChannelUsers mRegisteredUsers; /**< Users in this channel. */

        bool mPrivate;                 /**< Whether the channel is private. */
};

#endif
