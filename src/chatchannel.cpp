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

#include "chatchannel.h"

ChatChannel::ChatChannel(const std::string &channelName,
                         const std::string &channelAnnouncement = "None",
                         const std::string &channelPassword = "None"):
    mChannelName(channelName),
    mChannelAnnouncement(channelAnnouncement),
    mChannelPassword(channelPassword)
{
    if (channelAnnouncement == "")
        mChannelAnnouncement = "None";
    if (channelPassword == "")
        mChannelPassword = "None";
    mRegisteredUsers.clear();
}

ChatChannel::~ChatChannel()
{
    mRegisteredUsers.clear();
}


const std::string&
ChatChannel::getName() const
{
    return mChannelName;
}

const std::string&
ChatChannel::getAnnouncement() const
{
    return mChannelAnnouncement;
}

const std::string&
ChatChannel::getPassword() const
{
    return mChannelPassword;
}

void
ChatChannel::setName(const std::string &channelName)
{
    mChannelName = channelName;
}

void
ChatChannel::setAnnouncement(const std::string &channelAnnouncement)
{
    if (channelAnnouncement == "")
        mChannelAnnouncement = "None";
    else
        mChannelAnnouncement = channelAnnouncement;
}

void
ChatChannel::setPassword(const std::string &channelPassword)
{
    if (channelPassword == "")
        mChannelPassword = "None";
    else
        mChannelPassword = channelPassword;
}

ChatChannel::ChannelUsers const &ChatChannel::getUserList() const
{
    return mRegisteredUsers;
}


bool ChatChannel::addUserInChannel(std::string const &user)
{
    // Check if the user already exists in the channel
    ChannelUsers::const_iterator i = mRegisteredUsers.begin(),
                                 i_end = mRegisteredUsers.end();
    if (std::find(i, i_end, user) == i_end) return false;
    mRegisteredUsers.push_back(user);
    return true;
}


bool ChatChannel::removeUserFromChannel(std::string const &user)
{
    ChannelUsers::iterator i_end = mRegisteredUsers.end(),
                           i = std::find(mRegisteredUsers.begin(), i_end, user);
    if (i == i_end) return false;
    mRegisteredUsers.erase(i);
    return true;
}

void ChatChannel::removeEveryUsersFromChannel()
{
    mRegisteredUsers.clear();
}
