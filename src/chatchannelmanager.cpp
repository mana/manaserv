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

#include <map>

#include "chatchannelmanager.h"
#include "storage.h"

ChatChannelManager::ChatChannelManager()
{
    //Load stored public chat channels from db
    tmwserv::Storage &store = tmwserv::Storage::instance("tmw");
    mChatChannels = store.getChannelList();
}


ChatChannelManager::~ChatChannelManager()
{
    tmwserv::Storage &store = tmwserv::Storage::instance("tmw");
    store.updateChannels(mChatChannels);
    mChatChannels.clear();
}

short
ChatChannelManager::registerPublicChannel(const std::string& channelName,
    const std::string& channelAnnouncement,  const std::string& channelPassword)
{
    short channelId = 1;
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->second.getName() == channelName ) return 0;
        // We seek the highest channelId in the public range
        if ( (channelId <= i->first) && (i->first < (signed)MAX_PRIVATE_CHANNELS_RANGE) )
            channelId = i->first + 1;
        ++i;
    }
    // Too much channels registered
    if ( channelId >= (signed)MAX_PRIVATE_CHANNELS_RANGE ) return 0;

    // Register Channel
    mChatChannels.insert(std::make_pair(channelId,ChatChannel(channelName,
                                        channelAnnouncement, channelPassword)));
    return channelId;
}


short
ChatChannelManager::registerPrivateChannel(const std::string& channelName,
    const std::string& channelAnnouncement,  const std::string& channelPassword)
{
    short channelId = MAX_PRIVATE_CHANNELS_RANGE;
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->second.getName() == channelName ) return 0;
        // We seek the highest channelId in the private range
        if ( (channelId <= i->first) && (i->first >= (signed)MAX_PRIVATE_CHANNELS_RANGE) ) 
            channelId = i->first + 1;
        ++i;
    }
    // Too much channels registered
    if ( channelId >= (signed)MAX_PUBLIC_CHANNELS_RANGE ) return 0;

    // Register Channel
    mChatChannels.insert(std::make_pair(channelId,ChatChannel(channelName,
                                        channelAnnouncement, channelPassword)));
    return channelId;
}

bool
ChatChannelManager::removeChannel(const short channelId)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId )
        {
            // Removing every user from the channel
            i->second.removeEveryUsersFromChannel();
            // Erasing the channel now it's empty
            mChatChannels.erase(i);
            i++;
            return true;
        }
        ++i;
    }
    return false;
}


short
ChatChannelManager::getChannelId(const std::string& channelName)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->second.getName() == channelName ) return i->first;
        ++i;
    }
    return 0;
}


const std::string
ChatChannelManager::getChannelName(const short channelId)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId ) return i->second.getName();
        ++i;
    }
    return "";
}

const std::string
ChatChannelManager::getChannelAnnouncement(const short channelId)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId ) return i->second.getAnnouncement();
        ++i;
    }
    return "None";
}

const std::string
ChatChannelManager::getChannelPassword(const short channelId)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId ) return i->second.getPassword();
        ++i;
    }
    return "None";
}

bool
ChatChannelManager::setChannelAnnouncement(const short channelId, const std::string& channelAnnouncement)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId )
        {
            i->second.setAnnouncement(channelAnnouncement);
            return true;
        }
        ++i;
    }
    return false;
}

bool
ChatChannelManager::setChannelPassword(const short channelId, const std::string& channelPassword)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId )
        {
            i->second.setPassword(channelPassword);
            return true;
        }
        ++i;
    }
    return false;
}

const ChatChannel
ChatChannelManager::_getChannel(const short channelId)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId ) return i->second;
        ++i;
    }
    return ChatChannel("", "", "");
}


bool
ChatChannelManager::addUserInChannel(tmwserv::BeingPtr beingPtr, const short channelId)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId )
        {
            return i->second.addUserInChannel(beingPtr);
        }
        ++i;
    }
    return false;
}


bool
ChatChannelManager::removeUserFromChannel(tmwserv::BeingPtr beingPtr, const short channelId)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId )
        {
            return i->second.removeUserFromChannel(beingPtr);
        }
        ++i;
    }
    return false;
}

void
ChatChannelManager::removeUserFromEveryChannels(tmwserv::BeingPtr beingPtr)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        i->second.removeUserFromChannel(beingPtr);
        ++i;
    }
}

std::vector<tmwserv::BeingPtr>
ChatChannelManager::getUserListInChannel(const short channelId)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId )
        {
            return i->second.getUserList();
        }
        ++i;
    }
    std::vector<tmwserv::BeingPtr> emptyList;
    return emptyList;
}

bool
ChatChannelManager::isChannelRegistered(const short channelId)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->first == channelId ) return true;
        ++i;
    }
    return false;
}
