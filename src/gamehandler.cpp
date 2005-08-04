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

#include "gamehandler.h"
#include "messageout.h"
#include <iostream>

void GameHandler::receiveMessage(NetComputer &computer, MessageIn &message)
{
    if (computer.getCharacter() == NULL)
        return;

    MessageOut result;

    switch (message.getId())
    {
        case CMSG_PICKUP:
            {
                // add item to inventory (this is too simplistic atm)
                unsigned int itemId = message.readLong();

                // remove the item from world map

                // send feedback
                computer.getCharacter()->addInventory(itemId);
                result.writeShort(SMSG_PICKUP_RESPONSE);
                result.writeByte(PICKUP_OK);
            } break;
                
        case CMSG_USE_OBJECT:
            {
                unsigned int itemId = message.readLong();
                result.writeShort(SMSG_USE_RESPONSE);
                result.writeByte(USE_OK);
            } break;

        case CMSG_TARGET:
            break;

        case CMSG_WALK:
            break;

        case CMSG_START_TRADE:
            break;

        case CMSG_START_TALK:
            break;

        case CMSG_REQ_TRADE:
            break;

        case CMSG_USE_ITEM:
            break;

        case CMSG_EQUIP:
            break;

        default:
            std::cerr << "Warning: GameHandler received message of unkown type"
                      << " (" << message.getId() << ")" << std::endl;
            break;
    }

    computer.send(result.getPacket());
}
