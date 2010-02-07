/*
 *  The Mana Server
 *  Copyright (C) 2007-2010  The Mana World Development Team
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
    // general character properties
    msg.writeByte(data.getAccountLevel());
    msg.writeByte(data.getGender());
    msg.writeByte(data.getHairStyle());
    msg.writeByte(data.getHairColor());
    msg.writeShort(data.getLevel());
    msg.writeShort(data.getCharacterPoints());
    msg.writeShort(data.getCorrectionPoints());

    // character attributes
    for (int i = CHAR_ATTR_BEGIN; i < CHAR_ATTR_END; ++i)
    {
        msg.writeByte(data.getAttribute(i));
    }

    // character skills
    msg.writeShort(data.getSkillSize());

    std::map<int, int>::const_iterator skill_it;
    for (skill_it = data.getSkillBegin(); skill_it != data.getSkillEnd() ; skill_it++)
    {
        msg.writeShort(skill_it->first);
        msg.writeLong(skill_it->second);
    }

    // status effects currently affecting the character
    msg.writeShort(data.getStatusEffectSize());
    std::map<int, int>::const_iterator status_it;
    for (status_it = data.getStatusEffectBegin(); status_it != data.getStatusEffectEnd(); status_it++)
    {
        msg.writeShort(status_it->first);
        msg.writeShort(status_it->second);
    }

    // location
    msg.writeShort(data.getMapId());
    const Point &pos = data.getPosition();
    msg.writeShort(pos.x);
    msg.writeShort(pos.y);

    // kill count
    msg.writeShort(data.getKillCountSize());
    std::map<int, int>::const_iterator kills_it;
    for (kills_it = data.getKillCountBegin(); kills_it != data.getKillCountEnd(); kills_it++)
    {
        msg.writeShort(kills_it->first);
        msg.writeLong(kills_it->second);
    }

    // inventory - must be last because size isn't transmitted
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
    // general character properties
    data.setAccountLevel(msg.readByte());
    data.setGender(msg.readByte());
    data.setHairStyle(msg.readByte());
    data.setHairColor(msg.readByte());
    data.setLevel(msg.readShort());
    data.setCharacterPoints(msg.readShort());
    data.setCorrectionPoints(msg.readShort());

    // character attributes
    for (int i = CHAR_ATTR_BEGIN; i < CHAR_ATTR_END; ++i)
    {
        data.setAttribute(i, msg.readByte());
    }

    // character skills
    int skillSize = msg.readShort();

    for (int i = 0; i < skillSize; ++i)
    {
        int skill = msg.readShort();
        int level = msg.readLong();
        data.setExperience(skill,level);
    }

    // status effects currently affecting the character
    int statusSize = msg.readShort();

    for (int i = 0; i < statusSize; i++)
    {
        int status = msg.readShort();
        int time = msg.readShort();
        data.applyStatusEffect(status, time);
    }

    // location
    data.setMapId(msg.readShort());

    Point temporaryPoint;
    temporaryPoint.x = msg.readShort();
    temporaryPoint.y = msg.readShort();
    data.setPosition(temporaryPoint);

    // kill count
    int killSize = msg.readShort();
    for (int i = 0; i < killSize; i++)
    {
        int monsterId = msg.readShort();
        int kills = msg.readLong();
        data.setKillCount(monsterId, kills);
    }

    // inventory - must be last because size isn't transmitted
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
