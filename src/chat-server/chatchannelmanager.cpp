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
#include "chat-server/chathandler.hpp"
#include "chat-server/guildmanager.hpp"
#include "utils/stringfilter.h"

ChatChannelManager::ChatChannelManager() : mNextChannelId(1)
{
}


ChatChannelManager::~ChatChannelManager()
{
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

bool ChatChannelManager::tryNewPublicChannel(const std::string &name)
{
    if (!stringFilter->filterContent(name))
    {
        return false;
    }

    // Checking strings for length and double quotes
    if (name.empty() ||
        name.length() > MAX_CHANNEL_NAME ||
        stringFilter->findDoubleQuotes(name))
    {
        return false;
    }
    else if (guildManager->doesExist(name) || 
             channelExists(name))
    {
        // Channel already exists
        return false;
    }
    else
    {
        // We attempt to create a new channel
        short id = createNewChannel(name, "", "", true);
        return id ? true : false;
    }
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
        if (i->second.canJoin())
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
    for (ChatChannelIterator i = mChatChannels.begin();
         i != mChatChannels.end(); ++i)
    {
        if (i->second.getName() == name)
        {
            return &(i->second);
        }
    }

    return NULL;
}

void ChatChannelManager::setChannelTopic(int channelId, const std::string &topic)
{
    ChatChannelIterator i = mChatChannels.find(channelId);
    if(i == mChatChannels.end())
        return;

    i->second.setAnnouncement(topic);
    chatHandler->warnUsersAboutPlayerEventInChat(&(i->second),
                                                 topic,
                                                 CHAT_EVENT_TOPIC_CHANGE);
}

void ChatChannelManager::removeUserFromAllChannels(ChatClient *user)
{
    // Local copy as they will be destroyed under our feet.
    std::vector<ChatChannel *> channels = user->channels;
    // Reverse iterator to reduce load on vector operations.
    for (std::vector<ChatChannel *>::const_reverse_iterator
         i = channels.rbegin(), i_end = channels.rend(); i != i_end; ++i)
    {
        chatHandler->warnUsersAboutPlayerEventInChat((*i),
                                                     user->characterName,
                                                     CHAT_EVENT_LEAVING_PLAYER);
        (*i)->removeUser(user);
    }
}

bool ChatChannelManager::channelExists(int channelId)
{
    return mChatChannels.find(channelId) != mChatChannels.end();
}

bool ChatChannelManager::channelExists(const std::string &channelName)
{
    return getChannel(channelName) != NULL;
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
        channelId = mNextChannelId;
        ++mNextChannelId;
    }

    return channelId;
}
