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

#ifndef _TMWSERV_CHATCHANNELMANAGER_H_
#define _TMWSERV_CHATCHANNELMANAGER_H_

#include <map>

#include "chat-server/chatchannel.hpp"

class ChatChannelManager
{
    public:
        /**
         * Constructor.
         */
        ChatChannelManager();

        /**
         * Destructor.
         */
        ~ChatChannelManager();

        /**
         * Registers a public channel. Can fail if the maximum of public
         * channels has already been reached or when a channel with the same
         * name already exists.
         *
         * @return the ID of the registered channel, or 0 if the registering
         *         was unsuccessful.
         */
        short registerPublicChannel(const std::string &channelName,
                                    const std::string &channelAnnouncement,
                                    const std::string &channelPassword);

        /**
         * Registers a private channel. Can fail if the maximum of private
         * channels has already been reached or when a channel with the same
         * name already exists.
         *
         * TODO: Pretty much the same as registering public channel. Maybe they
         *       should be merged and private/public should be passed as a
         *       boolean?
         *
         * @return the ID of the registered channel, or 0 if the registering
         *         was unsuccessful.
         */
        short registerPrivateChannel(const std::string &channelName,
                                     const std::string &channelAnnouncement,
                                     const std::string &channelPassword);

        /**
         * Remove a channel.
         */
        bool removeChannel(short channelId);

        /**
         * Get all public channels.
         * TODO: Why not return an actual std::list?
         *
         * @param numChannels The number of channels returned is stored here
         * @return a list of channel names
         */
        std::string getPublicChannelNames(short &numChannels);

        /**
         * Get the number of channels that have been registered.
         * TODO: Documentation doesn't match function name, needs fixing.
         *
         * @return the number of registered channels
         */
        short getNumberOfChannelUsers(const std::string &channelName);

        /**
         * Get the id of a channel from its name.
         *
         * @return the id of the channel, 0 if it was unsuccessful.
         */
        short getChannelId(const std::string &channelName);

        /**
         * Get the name of a channel from its id.
         * TODO: Can probably return a const std::string&
         * TODO: Possibly throw exception when id doesn't exist
         *
         * @return the name of the channel
         */
        std::string getChannelName(short channelId);

        /**
         * Get the announcement string of a channel from its id.
         * TODO: Can probably return a const std::string&
         * TODO: Possibly throw exception when id doesn't exist
         *
         * @return the announcement string of the channel
         */
        std::string getChannelAnnouncement(short channelId);

        /**
         * Set the announcement string of a channel from its id.
         * TODO: Documentation about returned value is broken
         *
         * @return the announcement string of the channel
         */
        bool setChannelAnnouncement(short channelId,
                                    std::string const &channelAnnouncement);

        /**
         * Set the password of a channel by its id.
         * TODO: Documentation about returned value is broken
         *
         * @return the password of the channel
         */
        bool setChannelPassword(short channelId,
                                const std::string &channelPassword);

        /**
         * Get the password of a channel from its id.
         * TODO: Can probably return a const std::string &
         *
         * @return the password of the channel
         */
        std::string getChannelPassword(short channelId);

        /**
         * Get the privacy of the channel from its id.
         * TODO: Rename to isPrivate?
         *
         * @return the privacy of the channel
         */
        bool getChannelPrivacy(short channelId);

        /**
         * Get the ChatChannel object from its id.
         * TODO: If we have a channel object, why not use that to set
         *       announcement, password, private status, add/remove users, etc?
         *
         * @return the ChatChannel object
         */
        ChatChannel _getChannel(short channelId);

        /**
         * Add a user in a channel.
         */
        bool addUserInChannel(std::string const &, short channelId);

        /**
         * Remove a user from a channel.
         */
        bool removeUserFromChannel(std::string const &, short channelId);

        /**
         * Remove a user from all channels. Used at logout.
         */
        void removeUserFromAllChannels(std::string const &userName);

        /**
         * Get the list of the users registered in a channel.
         */
        std::vector<std::string> const &getUserListInChannel(short channelId);

        /**
         * Tells if a channel exists.
         */
        bool isChannelRegistered(short channelId);

    private:
        typedef std::map<short, ChatChannel> ChatChannels;
        typedef ChatChannels::iterator ChatChannelIterator;

        /**
         * The map keeping all the chat channels. The channel id must be
         * unique.
         */
        ChatChannels mChatChannels;
};

extern ChatChannelManager *chatChannelManager;

#endif
