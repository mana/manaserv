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
 */

#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <map>
#include <string>
#include <vector>

#include "net/netcomputer.hpp"

class ChatChannel;
class Guild;
class Party;

/**
 * A client connected to the chat server. Via this class, the chat server
 * keeps track of the character name and account level of a client.
 */
class ChatClient : public NetComputer
{
    public:
        /**
         * Constructor.
         */
        ChatClient(ENetPeer *peer):
            NetComputer(peer),
            party(0),
            accountLevel(0)
        {
        }

        std::string characterName;
        unsigned int characterId;
        std::vector< ChatChannel * > channels;
        Party* party;
        unsigned char accountLevel;
        std::map<ChatChannel*, std::string> userModes;
        int numInvites;
};

#endif
