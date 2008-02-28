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

ChatChannelManager::ChatChannelManager()
{
    // Load stored public chat channels from db
    mChatChannels = storage->getChannelList();
}


ChatChannelManager::~ChatChannelManager()
{
    storage->updateChannels(mChatChannels);
}

int
ChatChannelManager::registerPublicChannel(const std::string &channelName,
        const std::string &channelAnnouncement,
        const std::string &channelPassword)
{
    int channelId = 1;
    for (ChatChannelIterator i = mChatChannels.begin(),
         end = mChatChannels.end(); i != end; ++i)
    {
        // We seek the highest channelId in the public range
        if (channelId <= i->first &&
                i->first < MAX_PUBLIC_CHANNELS_RANGE)
        {
            channelId = i->first + 1;
        }
    }

    // Too many channels registered
    if (channelId >= MAX_PUBLIC_CHANNELS_RANGE)
        return 0;

    // Register channel
    mChatChannels.insert(std::make_pair(channelId,
                                        ChatChannel(channelId,
                                                    channelName,
                                                    channelAnnouncement,
                                                    channelPassword)));
    return channelId;
}


int
ChatChannelManager::registerPrivateChannel(const std::string &channelName,
        const std::string &channelAnnouncement,
        const std::string &channelPassword)
{
    int channelId = MAX_PUBLIC_CHANNELS_RANGE;

    for (ChatChannelIterator i = mChatChannels.begin(),
            end = mChatChannels.end(); i != end; ++i)
    {

        // We seek the highest channelId in the private range
        if (channelId <= i->first)
            channelId = i->first + 1;
    }

    // Too many channels registered
    if (channelId >= MAX_PRIVATE_CHANNELS_RANGE) 
        return 0;

    // Register Channel
    mChatChannels.insert(std::make_pair(channelId,
                                        ChatChannel(channelId,
                                                    channelName,
                                                    channelAnnouncement,
                                                    channelPassword)));
    return channelId;
}

bool ChatChannelManager::removeChannel(int channelId)
{
    ChatChannelIterator i = mChatChannels.find(channelId);
    if (i == mChatChannels.end()) return false;
    i->second.removeAllUsers();
    mChatChannels.erase(i);
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
