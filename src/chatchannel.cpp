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

ChatChannel::ChatChannel():
    mChannelId(0),
    mChannelName("")
{
    mRegisteredUsers.clear();
}

ChatChannel::ChatChannel(short channelId, std::string channelName):
    mChannelId(channelId),
    mChannelName(channelName)
{
    mRegisteredUsers.clear();
}

ChatChannel::~ChatChannel()
{
    mRegisteredUsers.clear();
}


const std::string
ChatChannel::getName()
{
    return mChannelName;
}


void
ChatChannel::setName(std::string channelName)
{
    mChannelName = channelName;
}


const short
ChatChannel::getChannelId()
{
    return mChannelId;
}


const std::list<std::string>
ChatChannel::getUserList()
{
    return mRegisteredUsers;
}


bool
ChatChannel::addUserInChannel(std::string playerName)
{
    // Check if the user already exists in the channel
    for (std::list<std::string>::iterator i = mRegisteredUsers.begin(); i != mRegisteredUsers.end();)
    {
        if ( *i == playerName ) return false;
        ++i;
    }
    mRegisteredUsers.push_back(playerName);
    return true;
}


bool
ChatChannel::removeUserFromChannel(std::string playerName)
{
    for (std::list<std::string>::iterator i = mRegisteredUsers.begin(); i != mRegisteredUsers.end();)
    {
        if ( *i == playerName )
        {
            mRegisteredUsers.erase(i);
            return true;
        }
        ++i;
    }
    return false;
}
