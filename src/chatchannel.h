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

#include <list>
#include <string>

class ChatChannel {

 public:

    /**
     * Constructors
     */
    ChatChannel();

    ChatChannel(short channelId, std::string channelName);

    /**
     * Destructor
     */
    ~ChatChannel();

    /**
     * Get the name of the channel
     */
    const std::string getName();

    /**
     * Set the name of the channel
     */
    void setName(std::string channelName);

    /**
     * Get the id of the channel
     */
    const short getChannelId();

    /**
     * Get the list of the users registered in the channel
     */
    const std::list<std::string> getUserList();

    /**
     * Add a user in the channel
     */
    bool addUserInChannel(std::string playerName);

    /**
     * Remove a user from the channel.
     */
    bool removeUserFromChannel(std::string playerName);

 private:

    /**
     * The channel id which must be unique.
     */
    short mChannelId;

    /**
     * The Channel's name
     */
    std::string mChannelName;

    /**
     * The registered user list
     */
    std::list<std::string> mRegisteredUsers;

};

#endif
