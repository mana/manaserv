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

#include <iostream>

#include "messagein.h"
#include "messageout.h"
#include "netcomputer.h"
#include "packet.h"

void GameHandler::receiveMessage(NetComputer &computer, MessageIn &message)
{
    if (computer.getCharacter().get() == NULL)
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

        case CMSG_USE_ITEM:
        case CMSG_USE_OBJECT:
            {
                unsigned int itemId = message.readLong();

                result.writeShort(SMSG_USE_RESPONSE);

                if (computer.getCharacter()->hasItem(itemId)) {
                    // use item
                    // this should execute a script which will do the appropriate action
                    // (the script will determine if the item is 1 use only)
                    result.writeByte(USE_OK);                    
                } else {
                    result.writeByte(USE_FAIL);
                }
            } break;

        case CMSG_TARGET:
            {
                // nothing at the moment
            } break;

        case CMSG_WALK:
            {
                long x = message.readLong();
                long y = message.readLong();

                // simplistic "teleport" walk
                computer.getCharacter()->setX(x);
                computer.getCharacter()->setY(y);

                // no response should be required
            } break;

        case CMSG_START_TRADE:
            {
                // nothing at the moment
            } break;

        case CMSG_START_TALK:
            {
                // nothing at the moment
            } break;

        case CMSG_REQ_TRADE:
            {
                // nothing at the moment
            } break;

        case CMSG_EQUIP:
            {
                int itemId = message.readLong();
                char slot = message.readByte();

                result.writeShort(SMSG_EQUIP_RESPONSE);
                result.writeByte(computer.getCharacter()->equip(itemId, slot) ?
                                 EQUIP_OK : EQUIP_FAIL);
            } break;

        default:
            std::cerr << "Warning: GameHandler received message of unkown type"
                      << " (" << message.getId() << ")" << std::endl;
            break;
    }

    if (result.getPacket()->length > 0)
        computer.send(result.getPacket());
}
