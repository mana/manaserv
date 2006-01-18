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

ChatChannel::ChatChannel(const std::string channelName,
                         const std::string channelAnnouncement = "",
                         const std::string channelPassword = ""):
    mChannelName(channelName),
    mChannelAnnouncement(channelAnnouncement),
    mChannelPassword(channelPassword)
{
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
ChatChannel::setName(const std::string channelName)
{
    mChannelName = channelName;
}

void
ChatChannel::setAnnouncement(const std::string channelAnnouncement)
{
    mChannelAnnouncement = channelAnnouncement;
}

void
ChatChannel::setPassword(const std::string channelPassword)
{
    mChannelPassword = channelPassword;
}

std::vector<tmwserv::BeingPtr>
ChatChannel::getUserList() const
{
    return mRegisteredUsers;
}


bool
ChatChannel::addUserInChannel(tmwserv::BeingPtr beingPtr)
{
    // Check if the user already exists in the channel
    for (std::vector<tmwserv::BeingPtr>::iterator i = mRegisteredUsers.begin(); i != mRegisteredUsers.end();)
    {
        if ( i->get() == beingPtr.get() ) return false;
        ++i;
    }
    mRegisteredUsers.push_back(beingPtr);
    return true;
}


bool
ChatChannel::removeUserFromChannel(tmwserv::BeingPtr beingPtr)
{
    for (std::vector<tmwserv::BeingPtr>::iterator i = mRegisteredUsers.begin(); i != mRegisteredUsers.end();)
    {
        if ( i->get() == beingPtr.get() )
        {
            mRegisteredUsers.erase(i);
            return true;
        }
        ++i;
    }
    return false;
}
