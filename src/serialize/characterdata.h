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

#ifndef SERIALIZE_CHARACTERDATA_H
#define SERIALIZE_CHARACTERDATA_H

#include <map>

#include "common/defines.h"
#include "common/inventorydata.h"
#include "common/manaserv_protocol.h"
#include "net/messagein.h"
#include "net/messageout.h"
#include "utils/point.h"

template< class T >
void serializeCharacterData(const T &data, MessageOut &msg)
{
    // general character properties
    msg.writeInt8(data.getAccountLevel());
    msg.writeInt8(data.getGender());
    msg.writeInt8(data.getHairStyle());
    msg.writeInt8(data.getHairColor());
    msg.writeInt16(data.getCharacterPoints());
    msg.writeInt16(data.getCorrectionPoints());


    const AttributeMap &attributes = data.getAttributes();
    msg.writeInt16(attributes.size());
    for (auto attributeIt : attributes)
    {
        msg.writeInt16(attributeIt.first);
        msg.writeDouble(attributeIt.second.getBase());
        msg.writeDouble(attributeIt.second.getModifiedAttribute());
    }

    // status effects currently affecting the character
    msg.writeInt16(data.getStatusEffectSize());
    std::map<int, Status>::const_iterator status_it;
    for (status_it = data.getStatusEffectBegin(); status_it != data.getStatusEffectEnd(); status_it++)
    {
        msg.writeInt16(status_it->first);
        msg.writeInt16(status_it->second.time);
    }

    // location
    msg.writeInt16(data.getMapId());
    const Point &pos = data.getPosition();
    msg.writeInt16(pos.x);
    msg.writeInt16(pos.y);

    // kill count
    msg.writeInt16(data.getKillCountSize());
    std::map<int, int>::const_iterator kills_it;
    for (kills_it = data.getKillCountBegin(); kills_it != data.getKillCountEnd(); kills_it++)
    {
        msg.writeInt16(kills_it->first);
        msg.writeInt32(kills_it->second);
    }

    // character abilities
    AbilityMap::const_iterator abilitiy_it;
    msg.writeInt16(data.getAbilitySize());
    for (abilitiy_it = data.getAbilityBegin();
         abilitiy_it != data.getAbilityEnd(); abilitiy_it++)
    {
        msg.writeInt32(abilitiy_it->first);
        msg.writeInt32(abilitiy_it->second.currentPoints);
    }

    // inventory - must be last because size isn't transmitted
    const Possessions &poss = data.getPossessions();
    const EquipData &equipData = poss.getEquipment();
    msg.writeInt16(equipData.size()); // number of equipment
    for (EquipData::const_iterator k = equipData.begin(),
             k_end = equipData.end(); k != k_end; ++k)
    {
        msg.writeInt16(k->first);                 // Equip slot id
        msg.writeInt16(k->second.itemId);         // ItemId
        msg.writeInt16(k->second.itemInstance);   // Item Instance id
    }

    const InventoryData &inventoryData = poss.getInventory();
    for (InventoryData::const_iterator j = inventoryData.begin(),
         j_end = inventoryData.end(); j != j_end; ++j)
    {
        msg.writeInt16(j->first);           // slot id
        msg.writeInt16(j->second.itemId);   // item id
        msg.writeInt16(j->second.amount);   // amount
    }
}

template< class T >
void deserializeCharacterData(T &data, MessageIn &msg)
{
    // general character properties
    data.setAccountLevel(msg.readInt8());
    data.setGender(ManaServ::getGender(msg.readInt8()));
    data.setHairStyle(msg.readInt8());
    data.setHairColor(msg.readInt8());
    data.setCharacterPoints(msg.readInt16());
    data.setCorrectionPoints(msg.readInt16());

    // character attributes
    unsigned attrSize = msg.readInt16();
    for (unsigned i = 0; i < attrSize; ++i)
    {
        unsigned id = msg.readInt16();
        double base = msg.readDouble(),
               mod  = msg.readDouble();
        data.setAttribute(id, base);
        data.setModAttribute(id, mod);
    }

    // status effects currently affecting the character
    int statusSize = msg.readInt16();

    for (int i = 0; i < statusSize; i++)
    {
        int status = msg.readInt16();
        int time = msg.readInt16();
        data.applyStatusEffect(status, time);
    }

    // location
    data.setMapId(msg.readInt16());

    Point temporaryPoint;
    temporaryPoint.x = msg.readInt16();
    temporaryPoint.y = msg.readInt16();
    data.setPosition(temporaryPoint);

    // kill count
    int killSize = msg.readInt16();
    for (int i = 0; i < killSize; i++)
    {
        int monsterId = msg.readInt16();
        int kills = msg.readInt32();
        data.setKillCount(monsterId, kills);
    }

    // character abilities
    int abilitiesSize = msg.readInt16();
    data.clearAbilities();
    for (int i = 0; i < abilitiesSize; i++)
    {
        const int id = msg.readInt32();
        const int mana = msg.readInt32();
        data.giveAbility(id, mana);
    }


    Possessions &poss = data.getPossessions();
    EquipData equipData;
    int equipSlotsSize = msg.readInt16();
    unsigned eqSlot;
    EquipmentItem equipItem;
    for (int j = 0; j < equipSlotsSize; ++j)
    {
        eqSlot  = msg.readInt16();
        equipItem.itemId = msg.readInt16();
        equipItem.itemInstance = msg.readInt16();
        equipData.insert(equipData.end(),
                               std::make_pair(eqSlot, equipItem));
    }
    poss.setEquipment(equipData);

    // Loads inventory - must be last because size isn't transmitted
    InventoryData inventoryData;
    while (msg.getUnreadLength())
    {
        InventoryItem i;
        int slotId = msg.readInt16();
        i.itemId   = msg.readInt16();
        i.amount   = msg.readInt16();
        inventoryData.insert(inventoryData.end(), std::make_pair(slotId, i));
    }
    poss.setInventory(inventoryData);
}

#endif // SERIALIZE_CHARACTERDATA_H
