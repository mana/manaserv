/*
 *  The Mana Server
 *  Copyright (C) 2007  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SERIALIZE_CHARACTERDATA_HPP
#define SERIALIZE_CHARACTERDATA_HPP

#include <map>

#include "defines.h"
#include "common/inventorydata.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "point.h"

template< class T >
void serializeCharacterData(const T &data, MessageOut &msg)
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

    msg.writeShort(data.getSkillSize());

    std::map<int, int>::const_iterator skill_it;
    for (skill_it = data.getSkillBegin(); skill_it != data.getSkillEnd() ; skill_it++)
    {
        msg.writeShort(skill_it->first);
        msg.writeLong(skill_it->second);
    }

    msg.writeShort(data.getStatusEffectSize());
    std::map<int, int>::const_iterator status_it;
    for (status_it = data.getStatusEffectBegin(); status_it != data.getStatusEffectEnd(); status_it++)
    {
        msg.writeShort(status_it->first);
        msg.writeShort(status_it->second);
    }


    msg.writeShort(data.getMapId());
    const Point &pos = data.getPosition();
    msg.writeShort(pos.x);
    msg.writeShort(pos.y);

    const Possessions &poss = data.getPossessions();
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

    int skillSize = msg.readShort();

    for (int i = 0; i < skillSize; ++i)
    {
        int skill = msg.readShort();
        int level = msg.readLong();
        data.setExperience(skill,level);
    }

    int statusSize = msg.readShort();

    for (int i = 0; i < statusSize; i++)
    {
        int status = msg.readShort();
        int time = msg.readShort();
        data.applyStatusEffect(status, time);
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
