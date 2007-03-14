/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */

#include "abstractcharacterdata.hpp"

#include "defines.h"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "point.h"

void AbstractCharacterData::serialize(MessageOut &msg) const
{
    msg.writeLong(getDatabaseID());
    msg.writeString(getName());
    msg.writeByte(getGender());
    msg.writeByte(getHairStyle());
    msg.writeByte(getHairColor());
    msg.writeByte(getLevel());
    msg.writeShort(getMoney());

    for (int i = 0; i < NB_BASE_ATTRIBUTES; ++i)
    {
        msg.writeByte(getBaseAttribute(i));
    }

    msg.writeShort(getMapId());
    msg.writeShort(getPosition().x);
    msg.writeShort(getPosition().y);

    /**
     * TODO: uncomment/redesign after Inventory is redesigned/improved
     * TODO: Test and debug.
     *
    for (int j = 0; j < getNumberOfInventoryItems(); ++j)
    {
        msg.writeShort(getInventoryItem(j).itemClassId);
        msg.writeShort(getInventoryItem(j).numberOfItemsInSlot);
        msg.writeByte((char)getInventoryItem(j).isEquiped);
    }
    */
}

void AbstractCharacterData::deserialize(MessageIn &msg)
{
    setDatabaseID(msg.readLong());
    setName(msg.readString());
    setGender(msg.readByte());
    setHairStyle(msg.readByte());
    setHairColor(msg.readByte());
    setLevel(msg.readByte());
    setMoney(msg.readShort());

    for (int i = 0; i < NB_BASE_ATTRIBUTES; ++i)
    {
        setBaseAttribute(i, msg.readByte());
    }

    setMapId(msg.readShort());

    Point temporaryPoint;
    temporaryPoint.x = msg.readShort();
    temporaryPoint.y = msg.readShort();
    setPosition(temporaryPoint);

    /**
     * TODO: uncomment/redesign after Inventory is redesigned/improved
     * TODO: Test and debug.
     *

    clearInventory();

    // byte = 1, short = 2
    while (msg.getUnreadLength() >= 5)
    {
        InventoryItem tempItem;
        tempItem.itemClassId = msg.readShort();
        tempItem.numberOfItemsInSlot = msg.readShort();
        tempItem.isEquiped = (bool) msg.readByte();
        addItemToInventory(tempItem);
    }
    */
}
