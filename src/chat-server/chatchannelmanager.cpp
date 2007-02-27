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

#include "account-server/storage.hpp"
#include "chat-server/chatchannelmanager.hpp"

ChatChannelManager::ChatChannelManager()
{
    //Load stored public chat channels from db
    Storage &store = Storage::instance("tmw");
    mChatChannels = store.getChannelList();
}


ChatChannelManager::~ChatChannelManager()
{
    Storage &store = Storage::instance("tmw");
    store.updateChannels(mChatChannels);
    mChatChannels.clear();
}

short
ChatChannelManager::registerPublicChannel(const std::string& channelName,
    const std::string& channelAnnouncement,  const std::string& channelPassword)
{
    short channelId = 1;
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(),
         end = mChatChannels.end(); i != end; ++i)
    {
        if ( i->second.getName() == channelName ) return 0;
        // We seek the highest channelId in the public range
        if (channelId <= i->first && i->first < (signed)MAX_PUBLIC_CHANNELS_RANGE)
            channelId = i->first + 1;
    }
    // Too much channels registered
    if (channelId >= (signed)MAX_PUBLIC_CHANNELS_RANGE) return 0;

    // Register Channel
    mChatChannels.insert(std::make_pair(channelId,ChatChannel(channelName,
                                        channelAnnouncement, channelPassword, false)));
    return channelId;
}


short
ChatChannelManager::registerPrivateChannel(const std::string& channelName,
    const std::string& channelAnnouncement,  const std::string& channelPassword)
{
    short channelId = MAX_PUBLIC_CHANNELS_RANGE;
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(),
         end = mChatChannels.end(); i != end; ++i)
    {
        if ( i->second.getName() == channelName ) return 0;
        // We seek the highest channelId in the private range
        if (channelId <= i->first)
            channelId = i->first + 1;
    }
    // Too much channels registered
    if (channelId >= (signed)MAX_PRIVATE_CHANNELS_RANGE) return 0;

    // Register Channel
    mChatChannels.insert(std::make_pair(channelId,ChatChannel(channelName,
                                        channelAnnouncement, channelPassword, true)));
    return channelId;
}

bool ChatChannelManager::removeChannel(short channelId)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    if (i == mChatChannels.end()) return false;
    i->second.removeEveryUsersFromChannel();
    mChatChannels.erase(i);
    return true;
}

std::string ChatChannelManager::getPublicChannelNames(short *numChannels)
{
    std::string channels;
    for (std::map<short, ChatChannel>::const_iterator i = mChatChannels.begin(), i_end = mChatChannels.end();
         i != i_end; ++i) {
        if(!i->second.getPrivacy())
        {
            channels.append(i->second.getName());
            channels += " ";
            (*numChannels)++;
        }
    }
    return channels;
}

short ChatChannelManager::getNumberOfChannelUsers(const std::string &channelName)
{
    ChatChannel channel = _getChannel(getChannelId(channelName));
	short size =  channel.getUserList().size();
    return size;
}

short ChatChannelManager::getChannelId(std::string const &channelName)
{
    for (std::map<short, ChatChannel>::const_iterator i = mChatChannels.begin(), i_end = mChatChannels.end();
         i != i_end; ++i) {
        if (i->second.getName() == channelName) return i->first;
    }
    return 0;
}


std::string ChatChannelManager::getChannelName(short channelId)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    return (i != mChatChannels.end()) ? i->second.getName() : std::string();
}

std::string ChatChannelManager::getChannelAnnouncement(short channelId)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    return (i != mChatChannels.end()) ? i->second.getAnnouncement() : std::string();
}

std::string ChatChannelManager::getChannelPassword(short channelId)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    return (i != mChatChannels.end()) ? i->second.getPassword() : std::string();
}

bool ChatChannelManager::getChannelPrivacy(short channelId)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    return (i != mChatChannels.end()) ? i->second.getPrivacy() : true;
}

bool ChatChannelManager::setChannelAnnouncement(short channelId, std::string const &channelAnnouncement)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    if (i == mChatChannels.end()) return false;
    i->second.setAnnouncement(channelAnnouncement);
    return true;
}

bool ChatChannelManager::setChannelPassword(short channelId, std::string const &channelPassword)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    if (i == mChatChannels.end()) return false;
    i->second.setPassword(channelPassword);
    return true;
}

ChatChannel ChatChannelManager::_getChannel(short channelId)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    if (i != mChatChannels.end()) return i->second;
    return ChatChannel("", "", "", true);
}


bool ChatChannelManager::addUserInChannel(std::string const &user, short channelId)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    if (i == mChatChannels.end()) return false;
    return i->second.addUserInChannel(user);
}


bool ChatChannelManager::removeUserFromChannel(std::string const &user, short channelId)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    if (i == mChatChannels.end()) return false;
    return i->second.removeUserFromChannel(user);
}

void ChatChannelManager::removeUserFromEveryChannels(std::string const &user)
{
    for (std::map<short, ChatChannel>::iterator i = mChatChannels.begin(), i_end = mChatChannels.end();
         i != i_end; ++i) {
         i->second.removeUserFromChannel(user);
    }
}

std::vector< std::string > const &
ChatChannelManager::getUserListInChannel(short channelId)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    if (i != mChatChannels.end()) return i->second.getUserList();
    static std::vector< std::string > emptyList;
    return emptyList;
}

bool ChatChannelManager::isChannelRegistered(short channelId)
{
    std::map<short, ChatChannel>::iterator i = mChatChannels.find(channelId);
    return i != mChatChannels.end();
}
