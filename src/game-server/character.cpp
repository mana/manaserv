/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
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

#include <algorithm>
#include <cassert>

#include "game-server/character.hpp"

#include "defines.h"
#include "game-server/buysell.hpp"
#include "game-server/eventlistener.hpp"
#include "game-server/inventory.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/trade.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "serialize/characterdata.hpp"

Character::Character(MessageIn &msg):
    Being(OBJECT_CHARACTER, 65535),
    mClient(NULL), mTransactionHandler(NULL), mDatabaseID(-1),
    mGender(0), mHairStyle(0), mHairColor(0), mLevel(1),
    mTransaction(TRANS_NONE)
{
    Attribute attr = { 0, 0 };
    mAttributes.resize(NB_CHARACTER_ATTRIBUTES, attr);
    // Get character data.
    mDatabaseID = msg.readLong();
    mName = msg.readString();
    deserializeCharacterData(*this, msg);
    for (int i = CHAR_ATTR_BEGIN; i < CHAR_ATTR_END; ++i)
    {
        modifiedAttribute(i);
    }
    setSize(16);
    Inventory(this).initialize();
}

void Character::perform()
{
    if (mAction != ATTACK || mActionTime > 0) return;

    mActionTime = 1000;
    mAction = STAND;
    raiseUpdateFlags(UPDATEFLAG_ATTACK);

    // TODO: Check slot 2 too.
    int itemId = mPossessions.equipment[EQUIP_FIGHT1_SLOT];
    ItemClass *ic = ItemManager::getItem(itemId);
    int type = ic ? ic->getModifiers().getValue(MOD_WEAPON_TYPE) : WPNTYPE_NONE;

    Damage damage;
    damage.base = getModifiedAttribute(BASE_ATTR_PHY_ATK_MIN);
    damage.delta = getModifiedAttribute(BASE_ATTR_PHY_ATK_DELTA);
    damage.type = DAMAGE_PHYSICAL;
    damage.cth = getModifiedAttribute(BASE_ATTR_HIT)
        + getModifiedAttribute(CHAR_SKILL_WEAPON_BEGIN + type);
    if (type)
    {
        ItemModifiers const &mods = ic->getModifiers();
        damage.element = mods.getValue(MOD_ELEMENT_TYPE);
    }
    else
    {
        // No-weapon fighting.
        damage.element = ELEMENT_NEUTRAL;
    }

    int attackRange = 60; //TODO: get from weapon
    int attackAngle = 30; //TODO: get from weapon

    performAttack(damage, attackRange, attackAngle);
}

int Character::getMapId() const
{
    return getMap()->getID();
}

void Character::setMapId(int id)
{
    setMap(MapManager::getMap(id));
}

void Character::cancelTransaction()
{
    TransactionType t = mTransaction;
    mTransaction = TRANS_NONE;
    switch (t)
    {
        case TRANS_TRADE:
            static_cast< Trade * >(mTransactionHandler)->cancel(this);
            break;
        case TRANS_BUYSELL:
            static_cast< BuySell * >(mTransactionHandler)->cancel();
            break;
        case TRANS_NONE:
            return;
    }
}

Trade *Character::getTrading() const
{
    return mTransaction == TRANS_TRADE
        ? static_cast< Trade * >(mTransactionHandler) : NULL;
}

BuySell *Character::getBuySell() const
{
    return mTransaction == TRANS_BUYSELL
        ? static_cast< BuySell * >(mTransactionHandler) : NULL;
}

void Character::setTrading(Trade *t)
{
    if (t)
    {
        cancelTransaction();
        mTransactionHandler = t;
        mTransaction = TRANS_TRADE;
    }
    else
    {
        assert(mTransaction == TRANS_NONE || mTransaction == TRANS_TRADE);
        mTransaction = TRANS_NONE;
    }
}

void Character::setBuySell(BuySell *t)
{
    if (t)
    {
        cancelTransaction();
        mTransactionHandler = t;
        mTransaction = TRANS_BUYSELL;
    }
    else
    {
        assert(mTransaction == TRANS_NONE || mTransaction == TRANS_BUYSELL);
        mTransaction = TRANS_NONE;
    }
}

void Character::sendStatus()
{
    if (mModifiedAttributes.empty()) return;

    MessageOut msg(GPMSG_PLAYER_ATTRIBUTE_CHANGE);
    for (std::vector< unsigned char >::const_iterator i = mModifiedAttributes.begin(),
         i_end = mModifiedAttributes.end(); i != i_end; ++i)
    {
        int attr = *i;
        msg.writeByte(attr);
        msg.writeShort(getAttribute(attr));
        msg.writeShort(getModifiedAttribute(attr));
    }
    gameHandler->sendTo(this, msg);

    mModifiedAttributes.clear();
}

void Character::modifiedAttribute(int attr)
{
    if (attr >= CHAR_ATTR_BEGIN && attr < CHAR_ATTR_END)
    {
        for (int i = BASE_ATTR_BEGIN; i < BASE_ATTR_END; ++i)
        {
            int newValue = getAttribute(i);

            if (i == BASE_ATTR_HP){
                newValue = (getModifiedAttribute(CHAR_ATTR_VITALITY) + 10)
                        * (mLevel + 10);
            }
            else if (i == BASE_ATTR_HIT) {
                newValue = getModifiedAttribute(CHAR_ATTR_DEXTERITY)
                        /* + skill in class of currently equipped weapon */;
            }
            else if (i == BASE_ATTR_EVADE) {
                newValue = getModifiedAttribute(CHAR_ATTR_AGILITY);
                        /* TODO: multiply with 10 / (10 * equip_weight)*/
            }
            else if (i == BASE_ATTR_PHY_RES) {
                newValue = getModifiedAttribute(CHAR_ATTR_VITALITY);
                        /* equip defence is through equip modifiers */
            }
            else if (i == BASE_ATTR_PHY_ATK_MIN) {
                newValue = getModifiedAttribute(CHAR_ATTR_STRENGTH);
                        /* weapon attack is applied through equip modifiers */
            }
            else if (i == BASE_ATTR_PHY_ATK_DELTA) {
                newValue =  0 /* + skill in class of currently equipped weapon */;
                        /* weapon attack is applied through equip modifiers */
            }
            else if (i == BASE_ATTR_MAG_RES) {
                newValue = getModifiedAttribute(CHAR_ATTR_WILLPOWER);
            }
            else if (i == BASE_ATTR_MAG_ATK) {
                newValue = getModifiedAttribute(CHAR_ATTR_WILLPOWER);
            }

            if (newValue != getAttribute(i))
            {
                setAttribute(i, newValue);
                flagAttribute(i);
            }
        }
    }
    flagAttribute(attr);
}

void Character::flagAttribute(int attr)
{
    // Warn the player of this attribute modification.
    std::vector< unsigned char >::iterator
        i_end = mModifiedAttributes.end(),
        i = std::find(mModifiedAttributes.begin(), i_end, (unsigned char)attr);
    if (i == i_end) mModifiedAttributes.push_back(attr);
}

void Character::disconnected()
{
    for (Listeners::iterator i = mListeners.begin(),
         i_end = mListeners.end(); i != i_end;)
    {
        EventListener const &l = **i;
        ++i; // In case the listener removes itself from the list on the fly.
        if (l.dispatch->disconnected) l.dispatch->disconnected(&l, this);
    }
}
