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

#include "chathandler.h"
#include "defines.h"
#include <iostream>

void ChatHandler::receiveMessage(NetComputer &computer, MessageIn &message)
{
    switch (message.getId())
    {
        case CMSG_SAY:
            {
                std::string text = message.readString();
                short channel = message.readShort();
                std::cout << "Say (" << channel << "): " << text << std::endl;
            }
            break;

        case CMSG_ANNOUNCE:
            {
                std::string text = message.readString();
                std::cout << "Announce: " << text << std::endl;
            }
            break;

        default:
            std::cout << "Invalid message type" << std::endl;
            break;
    }
}
