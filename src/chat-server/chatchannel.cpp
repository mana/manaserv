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

#include "chat-server/chatchannel.hpp"

ChatChannel::ChatChannel(short id,
                         const std::string &name,
                         const std::string &announcement,
                         const std::string &password,
                         bool privacy):
    mId(id),
    mName(name),
    mAnnouncement(announcement),
    mPassword(password),
    mPrivate(privacy)
{
    if (announcement == "")
        mAnnouncement = "None";
    if (password == "")
        mPassword = "None";
}

void
ChatChannel::setName(const std::string &name)
{
    mName = name;
}

void
ChatChannel::setAnnouncement(const std::string &announcement)
{
    if (announcement == "")
        mAnnouncement = "None";
    else
        mAnnouncement = announcement;
}

void
ChatChannel::setPassword(const std::string &password)
{
    if (password == "")
        mPassword = "None";
    else
        mPassword = password;
}

bool ChatChannel::addUser(const std::string &user)
{
    // Check if the user already exists in the channel
    ChannelUsers::const_iterator i = mRegisteredUsers.begin(),
                                 i_end = mRegisteredUsers.end();
    if (std::find(i, i_end, user) != i_end) return false;
    mRegisteredUsers.push_back(user);
    return true;
}

bool ChatChannel::removeUser(const std::string &user)
{
    ChannelUsers::iterator i_end = mRegisteredUsers.end(),
                           i = std::find(mRegisteredUsers.begin(), i_end, user);
    if (i == i_end) return false;
    mRegisteredUsers.erase(i);
    return true;
}

void ChatChannel::removeAllUsers()
{
    mRegisteredUsers.clear();
}
