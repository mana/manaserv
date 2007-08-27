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

#ifndef _TMWSERV_CHATCLIENT_H_
#define _TMWSERV_CHATCLIENT_H_

#include <string>
#include <vector>

#include "net/netcomputer.hpp"

class ChatChannel;

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
            NetComputer(peer)
        {
        }

        std::string characterName;
        std::vector< ChatChannel * > channels;
        unsigned char accountLevel;
};

#endif
