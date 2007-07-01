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

#ifndef _TMWSERV_SERIALIZE_CHARACTERDATA_HPP_
#define _TMWSERV_SERIALIZE_CHARACTERDATA_HPP_

#include "defines.h"
#include "common/inventorydata.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "point.h"

template< class T >
void serializeCharacterData(T const &data, MessageOut &msg)
{
    msg.writeLong(data.getDatabaseID());
    msg.writeString(data.getName());
    msg.writeByte(data.getGender());
    msg.writeByte(data.getHairStyle());
    msg.writeByte(data.getHairColor());
    msg.writeByte(data.getLevel());
    msg.writeShort(data.getMoney());

    for (int i = 0; i < NB_BASE_ATTRIBUTES; ++i)
    {
        msg.writeByte(data.getBaseAttribute(i));
    }

    msg.writeShort(data.getMapId());
    Point const &pos = data.getPosition();
    msg.writeShort(pos.x);
    msg.writeShort(pos.y);

    Possessions const &poss = data.getPossessions();
    for (int j = 0; j < EQUIPMENT_SLOTS; ++j)
    {
        msg.writeShort(poss.equipment[j]);
    }
    for (std::vector< InventoryItem >::const_iterator j = poss.inventory.begin(),
         j_end = poss.inventory.end(); j != j_end; ++j)
    {
        msg.writeShort(j->itemId);
        msg.writeByte(j->amount);
    }
}

template< class T >
void deserializeCharacterData(T &data, MessageIn &msg)
{
    data.setDatabaseID(msg.readLong());
    data.setName(msg.readString());
    data.setGender(msg.readByte());
    data.setHairStyle(msg.readByte());
    data.setHairColor(msg.readByte());
    data.setLevel(msg.readByte());
    data.setMoney(msg.readShort());

    for (int i = 0; i < NB_BASE_ATTRIBUTES; ++i)
    {
        data.setBaseAttribute(i, msg.readByte());
    }

    data.setMapId(msg.readShort());

    Point temporaryPoint;
    temporaryPoint.x = msg.readShort();
    temporaryPoint.y = msg.readShort();
    data.setPosition(temporaryPoint);

    Possessions &poss = data.getPossessions();
    for (int j = 0; j < EQUIPMENT_SLOTS; ++j)
    {
        poss.equipment[j] = msg.readShort();
    }
    poss.inventory.clear();
    while (msg.getUnreadLength())
    {
        InventoryItem i;
        i.itemId = msg.readShort();
        i.amount = msg.readByte();
        poss.inventory.push_back(i);
    }
}

#endif
