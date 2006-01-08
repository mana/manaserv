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


#include "being.h"


namespace tmwserv
{


PATH_NODE::PATH_NODE(unsigned short x, unsigned short y):
    x(x), y(y)
{
}

/**
 * Constructor.
 */
Being::Being(const std::string& name,
             const Genders gender,
             const unsigned short hairStyle,
             const unsigned short hairColor,
             const unsigned short level,
             const unsigned int money,
             const RawStatistics& stats)
        : mName(name),
          mGender(gender),
          mHairStyle(hairStyle),
          mHairColor(hairColor),
          mLevel(level),
          mMoney(money),
          mRawStats(stats)
{
    // NOOP
}


/**
 * Destructor.
 */
Being::~Being(void)
    throw()
{
    // NOOP
}


/**
 * Get the name.
 */
const std::string&
Being::getName(void) const
{
    return mName;
}


/**
 * Get the gender.
 */
Genders
Being::getGender(void) const
{
    return mGender;
}


/**
 * Get the Hair Style.
 */
unsigned short
Being::getHairStyle(void) const
{
    return mHairStyle;
}


/**
 * Get the Hair Color.
 */
unsigned short
Being::getHairColor(void) const
{
    return mHairColor;
}


/**
 * Set the level.
 */
void
Being::setLevel(const unsigned short level)
{
    mLevel = level;
}


/**
 * Get the level.
 */
unsigned short
Being::getLevel(void) const
{
    return mLevel;
}


/**
 * Set the money.
 */
void
Being::setMoney(const unsigned int amount)
{
    mMoney = amount;
}


/**
 * Get the amount of money.
 */
unsigned int
Being::getMoney(void) const
{
    return mMoney;
}


/**
 * Set the strength.
 */
void
Being::setStrength(const unsigned short strength)
{
    mRawStats.strength = strength;
    mNeedUpdate = true;
}


/**
 * Get the strength.
 */
unsigned short
Being::getStrength(void) const
{
    return mRawStats.strength;
}


/**
 * Set the agility.
 */
void
Being::setAgility(const unsigned short agility)
{
    mRawStats.agility = agility;
    mNeedUpdate = true;
}


/**
 * Get the agility.
 */
unsigned short
Being::getAgility(void) const
{
    return mRawStats.agility;
}


/**
 * Set the vitality.
 */
void
Being::setVitality(const unsigned short vitality)
{
    mRawStats.vitality = vitality;
    mNeedUpdate = true;
}


/**
 * Get the vitality.
 */
unsigned short
Being::getVitality(void) const
{
    return mRawStats.vitality;
}


/**
 * Set the intelligence.
 */
void
Being::setIntelligence(const unsigned short intelligence)
{
    mRawStats.intelligence = intelligence;
    mNeedUpdate = true;
}


/**
* Get the intelligence.
*
* @return the intelligence.
*/
unsigned short
Being::getIntelligence(void) const
{
    return mRawStats.intelligence;
}


/**
 * Set the dexterity.
 */
void
Being::setDexterity(const unsigned short dexterity)
{
    mRawStats.dexterity = dexterity;
    mNeedUpdate = true;
}


/**
 * Get the dexterity.
 */
unsigned short
Being::getDexterity(void) const
{
    return mRawStats.dexterity;
}


/**
 * Set the luck.
 */
void
Being::setLuck(const unsigned short luck)
{
    mRawStats.luck = luck;
    mNeedUpdate = true;
}


/**
 * Get the luck.
 */
unsigned short
Being::getLuck(void) const
{
    return mRawStats.luck;
}


/**
 * Set the raw statistics.
 */
void
Being::setRawStatistics(const RawStatistics& stats)
{
    mRawStats = stats;
    mNeedUpdate = true;
}


/**
 * Get the raw statistics.
 */
RawStatistics&
Being::getRawStatistics(void)
{
    return mRawStats;
}


/**
 * Update the internal status.
 */
void
Being::update(void)
{
    // computed stats.
    mStats.health = 20 + (20 * mRawStats.vitality);
    mStats.attack = 10 + mRawStats.strength;
    mStats.defense = 10 + mRawStats.strength;
    mStats.magic = 10 + mRawStats.intelligence;
    mStats.accuracy = 50 + mRawStats.dexterity;
    mStats.speed = mRawStats.dexterity;

    mNeedUpdate = false;
}

void
Being::setInventory(const std::vector<unsigned int> &inven)
{
    inventory = inven;
}

bool
Being::addInventory(unsigned int itemId)
{
    // If required weight could be tallied to see if player can pick up more.
    inventory.push_back(itemId);
    return true;
}

bool
Being::delInventory(unsigned int itemId)
{
    for (std::vector<unsigned int>::iterator i = inventory.begin();
         i != inventory.end(); i++) {
        if (*i == itemId) {
            inventory.erase(i);
            return true;
        }
    }
    return false;
}

bool
Being::hasItem(unsigned int itemId)
{
    for (std::vector<unsigned int>::iterator i = inventory.begin();
         i != inventory.end(); i++)
    {
        if (*i == itemId)
            return true;
    }
    return false;
}

bool
Being::equip(unsigned int itemId, unsigned char slot)
{
    // currently this is too simplistic and doesn't check enough
    // but until further functionality is implemented in the
    // server it will suffice
    if (slot < MAX_EQUIP_SLOTS) {
        equipment[slot] = itemId;
        return true;
    } else
        return false;
}

bool
Being::unequip(unsigned char slot)
{
    // NOTE: 0 will be invalid item id (or we could use key/value pairs)
    equipment[slot] = 0;
    return true;
}

} // namespace tmwserv
