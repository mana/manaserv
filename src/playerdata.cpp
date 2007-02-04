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

#include "playerdata.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"

PlayerData::PlayerData(std::string const &name, int id):
    mAccountID(-1),
    mDatabaseID(id),
    mName(name)
{
    for (int j = 0; j < EQUIPMENT_SLOTS; ++j)
    {
        mPossessions.equipment[j] = 0;
    }
}

void PlayerData::serialize(MessageOut &msg) const
{
    msg.writeByte(mGender);
    msg.writeByte(mHairStyle);
    msg.writeByte(mHairColor);
    msg.writeByte(mLevel);
    msg.writeShort(mMoney);
    for (int j = 0; j < NB_RSTAT; ++j)
    {
        msg.writeByte(mRawStats.stats[j]);
    }
    msg.writeShort(mMapId);
    msg.writeShort(mPos.x);
    msg.writeShort(mPos.y);
    for (int j = 0; j < EQUIPMENT_SLOTS; ++j)
    {
        msg.writeShort(mPossessions.equipment[j]);
    }
    for (std::vector< InventoryItem >::const_iterator j = mPossessions.inventory.begin(),
         j_end = mPossessions.inventory.end(); j != j_end; ++j)
    {
        msg.writeShort(j->itemId);
        msg.writeByte(j->amount);
    }
}

void PlayerData::deserialize(MessageIn &msg)
{
    mGender = msg.readByte();
    mHairStyle = msg.readByte();
    mHairColor = msg.readByte();
    mLevel = msg.readByte();
    mMoney = msg.readShort();
    for (int j = 0; j < NB_RSTAT; ++j)
    {
        mRawStats.stats[j] = msg.readByte();
    }
    mMapId = msg.readShort();
    mPos.x = msg.readShort();
    mPos.y = msg.readShort();
    for (int j = 0; j < EQUIPMENT_SLOTS; ++j)
    {
        mPossessions.equipment[j] = msg.readShort();
    }
    mPossessions.inventory.clear();
    while (msg.getUnreadLength())
    {
        InventoryItem i;
        i.itemId = msg.readShort();
        i.amount = msg.readByte();
        mPossessions.inventory.push_back(i);
    }
}
