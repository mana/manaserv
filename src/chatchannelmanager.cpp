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
    std::map<short, std::string> channelList = store.getChannelList();

    for (std::map<short, std::string>::iterator i = channelList.begin(); i != channelList.end();)
    {
        mChatChannels.push_back(ChatChannel(i->first,i->second));
        ++i;
    }
}


ChatChannelManager::~ChatChannelManager()
{
    tmwserv::Storage &store = tmwserv::Storage::instance("tmw");
    std::map<short, std::string> channelList;
    for (std::list<ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        channelList.insert(std::make_pair(i->getChannelId(), i->getName()));
        ++i;
    }
    store.updateChannels(channelList);
    mChatChannels.clear();
}

short
ChatChannelManager::registerPublicChannel(std::string channelName)
{
    short channelId = 1;
    for (std::list<ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->getName() == channelName ) return 0;
        // We seek the highest channelId in the public range
        if ( (channelId <= i->getChannelId()) && (i->getChannelId() < 1000) )
            channelId = i->getChannelId() + 1;
        ++i;
    }
    // Too much channels registered
    if ( channelId >= 1000 ) return 0;

    // Register Channel
    mChatChannels.push_back(ChatChannel(channelId,channelName));
    return channelId;
}


short
ChatChannelManager::registerPrivateChannel(std::string channelName)
{
    short channelId = 1000;
    for (std::list<ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->getName() == channelName ) return 0;
        // We seek the highest channelId in the private range
        if ( (channelId <= i->getChannelId()) && (i->getChannelId() >= 1000) ) 
            channelId = i->getChannelId() + 1;
        ++i;
    }
    // Too much channels registered
    if ( channelId >= 10000 ) return 0;

    // Register Channel
    mChatChannels.push_back(ChatChannel(channelId,channelName));
    return channelId;
}

bool
ChatChannelManager::removeChannel(short channelId)
{
    for (std::list<ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->getChannelId() == channelId )
        {
            mChatChannels.erase(i);
            i++;
            return true;
        }
        ++i;
    }
    return false;
}


short
ChatChannelManager::getChannelId(std::string channelName)
{
    for (std::list<ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->getName() == channelName ) return i->getChannelId();
        ++i;
    }
    return 0;
}


std::string
ChatChannelManager::getChannelName(short channelId)
{
    for (std::list<ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->getChannelId() == channelId ) return i->getName();
        ++i;
    }
    return "";
}


bool
ChatChannelManager::addUserInChannel(std::string playerName, short channelId)
{
    for (std::list<ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->getChannelId() == channelId )
        {
            return i->addUserInChannel(playerName);
        }
        ++i;
    }
    return false;
}


bool
ChatChannelManager::removeUserFromChannel(std::string playerName, short channelId)
{
    for (std::list<ChatChannel>::iterator i = mChatChannels.begin(); i != mChatChannels.end();)
    {
        if ( i->getChannelId() == channelId )
        {
            return i->removeUserFromChannel(playerName);
        }
        ++i;
    }
    return false;
}
