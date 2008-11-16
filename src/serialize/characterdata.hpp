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
    msg.writeByte(data.getAccountLevel());
    msg.writeByte(data.getGender());
    msg.writeByte(data.getHairStyle());
    msg.writeByte(data.getHairColor());
    msg.writeShort(data.getLevel());
    msg.writeShort(data.getCharacterPoints());
    msg.writeShort(data.getCorrectionPoints());

    for (int i = CHAR_ATTR_BEGIN; i < CHAR_ATTR_END; ++i)
    {
        msg.writeByte(data.getAttribute(i));
    }

    for (int i = 0; i < CHAR_SKILL_NB; ++i)
    {
        msg.writeLong(data.getExperience(i));
    }


    msg.writeShort(data.getMapId());
    Point const &pos = data.getPosition();
    msg.writeShort(pos.x);
    msg.writeShort(pos.y);

    Possessions const &poss = data.getPossessions();
    msg.writeLong(poss.money);
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
    data.setAccountLevel(msg.readByte());
    data.setGender(msg.readByte());
    data.setHairStyle(msg.readByte());
    data.setHairColor(msg.readByte());
    data.setLevel(msg.readShort());
    data.setCharacterPoints(msg.readShort());
    data.setCorrectionPoints(msg.readShort());

    for (int i = CHAR_ATTR_BEGIN; i < CHAR_ATTR_END; ++i)
    {
        data.setAttribute(i, msg.readByte());
    }

    for (int i = 0; i < CHAR_SKILL_NB; ++i)
    {
        data.setExperience(i, msg.readLong());
    }

    data.setMapId(msg.readShort());

    Point temporaryPoint;
    temporaryPoint.x = msg.readShort();
    temporaryPoint.y = msg.readShort();
    data.setPosition(temporaryPoint);

    Possessions &poss = data.getPossessions();
    poss.money = msg.readLong();
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
