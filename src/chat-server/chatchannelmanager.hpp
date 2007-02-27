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

#ifndef _TMWSERV_CHATCHANNELHANDLER_H_
#define _TMWSERV_CHATCHANNELHANDLER_H_

#include <map>

#include "chat-server/chatchannel.hpp"

class ChatChannelManager {

public:

    /**
     * Constructor
     */
    ChatChannelManager();

    /**
     * Destructor
     */
    ~ChatChannelManager();

    /**
     * Add a public channel.
     *
     * @return the number of the channel registered.
     * 0 if the registering was unsuccessful.
     */
    short registerPublicChannel(const std::string& channelName,
                                const std::string& channelAnnouncement,
                                const std::string& channelPassword);

    /**
     * Add a private channel.
     *
     * @return the number of the channel registered.
     * 0 if the registering was unsuccessful.
     */
    short registerPrivateChannel(const std::string& channelName,
                                 const std::string& channelAnnouncement,
                                 const std::string& channelPassword);

    /**
     * Remove a channel.
     */
    bool removeChannel(short channelId);

    /**
     * Get all public channels
     *
     * @return a list of channel names
     */
    std::string getPublicChannelNames(short *numChannels);

    /**
     * Get the number of channels that have been registered
     *
     * @return the number of registered channels
     */
    short getNumberOfChannelUsers(const std::string &channelName);

    /**
     * Get the id of a channel from its name.
     *
     * @return the id of the channel
     * 0 if it was unsuccessful.
     */
    short getChannelId(const std::string& channelName);

    /**
     * Get the name of a channel from its id.
     *
     * @return the name of the channel
     */
    std::string getChannelName(short channelId);

    /**
     * Get the announcement string of a channel from its id.
     *
     * @return the announcement string of the channel
     */
    std::string getChannelAnnouncement(short channelId);

    /**
     * Set the announcement string of a channel from its id.
     *
     * @return the announcement string of the channel
     */
    bool setChannelAnnouncement(short channelId, std::string const &channelAnnouncement);

    /**
     * Set the announcement string of a channel from its id.
     *
     * @return the announcement string of the channel
     */
    bool setChannelPassword(short channelId, const std::string& channelPassword);

    /**
     * Get the password of a channel from its id.
     *
     * @return the password of the channel
     */
    std::string getChannelPassword(short channelId);

    /**
     * Get the privacy of the channel from its id.
     *
     * @return the privacy of the channel
     */
    bool getChannelPrivacy(short channelId);

    /**
     * get the ChatChannel object from its id.
     *
     * @return the ChatChannel object
     */
    ChatChannel _getChannel(short channelId);

    /**
     * Add a user in a channel
     */
    bool addUserInChannel(std::string const &, short channelId);

    /**
     * Remove a user from a channel.
     */
    bool removeUserFromChannel(std::string const &, short channelId);

    /**
     * Remove a user from every channels.
     * Used at logout.
     */
    void removeUserFromEveryChannels(std::string const &);

    /**
     * Get the list of the users registered in a channel
     */
    std::vector< std::string > const &getUserListInChannel(short channelId);

    /**
     * tells if a channel exists
     */
    bool isChannelRegistered(short channelId);

private:

    /**
     * The list keeping all the chat channels.
     *
     * The channel id must be unique.
     */
    std::map<short, ChatChannel> mChatChannels;

};

extern ChatChannelManager *chatChannelManager;

#endif
