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

#include <list>

#include "chat-server/chatchannelmanager.hpp"

#include "defines.h"
#include "account-server/dalstorage.hpp"
#include "chat-server/chatclient.hpp"

ChatChannelManager::ChatChannelManager() : mNextChannelId(0)
{
    // Load stored public chat channels from db
    mChatChannels = storage->getChannelList();
}


ChatChannelManager::~ChatChannelManager()
{
    storage->updateChannels(mChatChannels);
}

int
ChatChannelManager::createNewChannel(const std::string &channelName,
        const std::string &channelAnnouncement,
        const std::string &channelPassword,
        bool joinable)
{
    int channelId = nextUsable();

    // Register channel
    mChatChannels.insert(std::make_pair(channelId,
                                        ChatChannel(channelId,
                                                    channelName,
                                                    channelAnnouncement,
                                                    channelPassword,
                                                    joinable)));
    return channelId;
}

bool ChatChannelManager::removeChannel(int channelId)
{
    ChatChannelIterator i = mChatChannels.find(channelId);
    if (i == mChatChannels.end()) return false;
    i->second.removeAllUsers();
    mChatChannels.erase(i);
    mChannelsNoLongerUsed.push_back(channelId);
    return true;
}

std::list<const ChatChannel*> ChatChannelManager::getPublicChannels()
{
    std::list<const ChatChannel*> channels;

    for (ChatChannels::const_iterator i = mChatChannels.begin(),
            i_end = mChatChannels.end();
         i != i_end; ++i)
    {
        if (!i->second.isPrivate())
        {
            channels.push_back(&i->second);
        }
    }

    return channels;
}

int ChatChannelManager::getChannelId(std::string const &channelName)
{
    for (ChatChannels::const_iterator i = mChatChannels.begin(),
            i_end = mChatChannels.end();
         i != i_end; ++i)
    {
        if (i->second.getName() == channelName) return i->first;
    }
    return 0;
}

ChatChannel* ChatChannelManager::getChannel(int channelId)
{
    ChatChannelIterator i = mChatChannels.find(channelId);
    if (i != mChatChannels.end()) return &i->second;
    return NULL;
}

ChatChannel* ChatChannelManager::getChannel(const std::string &name)
{
    ChatChannelIterator i_end = mChatChannels.end();
    for (ChatChannelIterator i = mChatChannels.begin(); i != i_end; ++i)
    {
        if (i->second.getName() == name)
        {
            return &(i->second);
        }
    }

    return NULL;
}

void ChatChannelManager::removeUserFromAllChannels(ChatClient *user)
{
    // Local copy as they will be destroyed under our feet.
    std::vector<ChatChannel *> channels = user->channels;
    // Reverse iterator to reduce load on vector operations.
    for (std::vector<ChatChannel *>::const_reverse_iterator
         i = channels.rbegin(), i_end = channels.rend(); i != i_end; ++i)
    {
         (*i)->removeUser(user);
    }
}

bool ChatChannelManager::channelExists(int channelId)
{
    return mChatChannels.find(channelId) != mChatChannels.end();
}

int ChatChannelManager::nextUsable()
{
    int channelId = 0;

    if (mChannelsNoLongerUsed.size() > 0)
    {
        channelId = mChannelsNoLongerUsed[0];
        mChannelsNoLongerUsed.pop_front();
    }
    else
    {
        channelId = ++mNextChannelId;
    }

    return channelId;
}
