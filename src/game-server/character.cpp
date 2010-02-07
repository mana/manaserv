/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits.h>

#include "game-server/character.hpp"

#include "common/configuration.hpp"
#include "game-server/accountconnection.hpp"
#include "game-server/buysell.hpp"
#include "game-server/eventlistener.hpp"
#include "game-server/inventory.hpp"
#include "game-server/item.hpp"
#include "game-server/itemmanager.hpp"
#include "game-server/gamehandler.hpp"
#include "game-server/mapcomposite.hpp"
#include "game-server/mapmanager.hpp"
#include "game-server/state.hpp"
#include "game-server/trade.hpp"
#include "scripting/script.hpp"
#include "net/messagein.hpp"
#include "net/messageout.hpp"
#include "serialize/characterdata.hpp"

#include "utils/logger.h"

// These values should maybe be obtained from the config file
const float Character::EXPCURVE_EXPONENT = 3.0f;
const float Character::EXPCURVE_FACTOR = 10.0f;
const float Character::LEVEL_SKILL_PRECEDENCE_FACTOR = 0.75f;
const float Character::EXP_LEVEL_FLEXIBILITY = 1.0f;

Character::Character(MessageIn &msg):
    Being(OBJECT_CHARACTER),
    mClient(NULL),
    mTransactionHandler(NULL),
    mRechargePerSpecial(0),
    mSpecialUpdateNeeded(false),
    mDatabaseID(-1),
    mGender(0),
    mHairStyle(0),
    mHairColor(0),
    mLevel(1),
    mLevelProgress(0),
    mUpdateLevelProgress(false),
    mRecalculateLevel(true),
    mParty(0),
    mTransaction(TRANS_NONE)
{
    Attribute attr = { 0, 0 };
    mAttributes.resize(CHAR_ATTR_NB, attr);
    // Get character data.
    mDatabaseID = msg.readLong();
    setName(msg.readString());
    deserializeCharacterData(*this, msg);
    for (int i = CHAR_ATTR_BEGIN; i < CHAR_ATTR_END; ++i)
    {
        modifiedAttribute(i);
    }
    setSize(16);
    Inventory(this).initialize();

    //give the character some specials for testing.
    //TODO: get from quest vars and equipment
    giveSpecial(1);
    giveSpecial(2);
    giveSpecial(3);

}

void Character::update()
{
    //update character level
    if (mRecalculateLevel)
    {
        mRecalculateLevel = false;
        recalculateLevel();
    }

    //update special recharge
    std::list<Special *> rechargeNeeded;
    int numRechargeNeeded = 0;
    for (std::map<int, Special*>::iterator i = mSpecials.begin(); i != mSpecials.end(); i++)
    {
        Special * s = i->second;
        if (s->currentMana < s->neededMana)
        {
            rechargeNeeded.push_back(s);
            numRechargeNeeded++;
        }
    }
    if (numRechargeNeeded > 0)
    {
        mRechargePerSpecial = getModifiedAttribute(CHAR_ATTR_INTELLIGENCE) / numRechargeNeeded;
        for (std::list<Special*>::iterator i = rechargeNeeded.begin(); i != rechargeNeeded.end(); i++)
        {
            (*i)->currentMana += mRechargePerSpecial;
        }
    }

    if (mSpecialUpdateNeeded)
    {
        sendSpecialUpdate();
        mSpecialUpdateNeeded = false;
    }

    mStatusEffects.clear();
    StatusEffects::iterator it = mStatus.begin();
    while (it != mStatus.end())
    {
        mStatusEffects[it->first] = it->second.time;
        it++;
    }
    Being::update();
}

void Character::perform()
{
    if (mAction != ATTACK || mTarget == NULL) return;

    // wait before next attack
    if (mActionTime > 100)
    {
        mActionTime -= 100;
        return;
    }

    // TODO: Check slot 2 too.
    int itemId = mPossessions.equipment[EQUIP_FIGHT1_SLOT];
    ItemClass *ic = ItemManager::getItem(itemId);
    int type = ic ? ic->getModifiers().getValue(MOD_WEAPON_TYPE) : 100;

    Damage damage;
    damage.base = getModifiedAttribute(BASE_ATTR_PHY_ATK_MIN);
    damage.delta = getModifiedAttribute(BASE_ATTR_PHY_ATK_DELTA) +
                   getModifiedAttribute(type);
    damage.type = DAMAGE_PHYSICAL;
    damage.cth = getModifiedAttribute(BASE_ATTR_HIT) +
                 getModifiedAttribute(type);
    damage.usedSkills.push_back(type);

    if (ic)
    {
        // weapon fighting
        const ItemModifiers &mods = ic->getModifiers();
        damage.element = mods.getValue(MOD_ELEMENT_TYPE);
        // todo: get attack range of weapon
        // (weapon equipping has to be fixed first)
        performAttack(mTarget, 64, damage);
    }
    else
    {
        // No-weapon fighting.
        damage.element = ELEMENT_NEUTRAL;
        performAttack(mTarget, 32, damage);
    }

}

void Character::respawn()
{
    if (mAction != DEAD)
    {
        LOG_WARN("Character \""<<getName()<<"\" tried to respawn without being dead");
        return;
    }

    //warp back to spawn point
    int spawnMap = Configuration::getValue("respawnMap", 1);
    int spawnX = Configuration::getValue("respawnX", 1024);
    int spawnY = Configuration::getValue("respawnY", 1024);
    GameState::enqueueWarp(this, MapManager::getMap(spawnMap), spawnX, spawnY);

    //make alive again
    setAction(STAND);
    mAttributes[BASE_ATTR_HP].mod = -mAttributes[BASE_ATTR_HP].base + 1;
    modifiedAttribute(BASE_ATTR_HP);

    // reset target
    mTarget = NULL;
}

void Character::useSpecial(int id)
{
    //check if the character may use this special in general
    std::map<int, Special*>::iterator i = mSpecials.find(id);
    if (i == mSpecials.end())
    {
        LOG_INFO("Character uses special "<<id<<" without autorisation.");
        return;
    }

    //check if the special is currently recharged
    Special *special = i->second;
    if (special->currentMana < special->neededMana)
    {
        LOG_INFO("Character uses special "<<id<<" which is not recharged. ("
                 <<special->currentMana<<"/"<<special->neededMana<<")");
        return;
    }

    //tell script engine to cast the spell
    special->currentMana = 0;
    Script *script = getMap()->getScript();
    if (script) {
        script->prepare("cast");
        script->push(this);
        script->push(id);
        script->execute();
    }

    mSpecialUpdateNeeded = true;
    return;
}

void Character::sendSpecialUpdate()
{
    //GPMSG_SPECIAL_STATUS           = 0x0293, // { B specialID, L current, L max, L recharge }
    for (std::map<int, Special*>::iterator i = mSpecials.begin();
         i != mSpecials.end();
         i++)
    {

        MessageOut msg(GPMSG_SPECIAL_STATUS );
        msg.writeByte(i->first);
        msg.writeLong(i->second->currentMana);
        msg.writeLong(i->second->neededMana);
        msg.writeLong(mRechargePerSpecial);
        /* yes, the last one is redundant because it is the same for each
           special, but I would like to keep the netcode flexible enough
           to allow different recharge speed per special when necessary */
        gameHandler->sendTo(this, msg);
    }
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
            static_cast< Trade * >(mTransactionHandler)->cancel();
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
    MessageOut attribMsg(GPMSG_PLAYER_ATTRIBUTE_CHANGE);
    for (std::set<size_t>::const_iterator i = mModifiedAttributes.begin(),
         i_end = mModifiedAttributes.end(); i != i_end; ++i)
    {
        int attr = *i;
        attribMsg.writeShort(attr);
        attribMsg.writeShort(getAttribute(attr));
        attribMsg.writeShort(getModifiedAttribute(attr));
    }
    if (attribMsg.getLength() > 2) gameHandler->sendTo(this, attribMsg);
    mModifiedAttributes.clear();

    MessageOut expMsg(GPMSG_PLAYER_EXP_CHANGE);
    for (std::set<size_t>::const_iterator i = mModifiedExperience.begin(),
         i_end = mModifiedExperience.end(); i != i_end; ++i)
    {
        int skill = *i;
        expMsg.writeShort(skill);
        expMsg.writeLong(getExpGot(skill));
        expMsg.writeLong(getExpNeeded(skill));
    }
    if (expMsg.getLength() > 2) gameHandler->sendTo(this, expMsg);
    mModifiedExperience.clear();

    if (mUpdateLevelProgress)
    {
        mUpdateLevelProgress = false;
        MessageOut progressMessage(GPMSG_LEVEL_PROGRESS);
        progressMessage.writeByte(mLevelProgress);
        gameHandler->sendTo(this, progressMessage);
    }
}

int Character::getAttribute(int attr) const
{
    if (attr <= CHAR_ATTR_END)
    {
        return Being::getAttribute(attr);
    }
    else
    {
        return  Character::levelForExp(mExperience.find(attr)->second);
    }
}

int Character::getModifiedAttribute(int attr) const
{
    if (attr <= CHAR_ATTR_END)
    {
        return Being::getModifiedAttribute(attr);
    }
    else
    {
        //TODO: Find a way to modify skills
        return  Character::levelForExp(mExperience.find(attr)->second);
    }
}

void Character::modifiedAttribute(int attr)
{
    if (attr >= CHAR_ATTR_BEGIN && attr < CHAR_ATTR_END)
    {
        for (int i = BASE_ATTR_BEGIN; i < BASE_ATTR_END; ++i)
        {
            int newValue = getAttribute(i);

            if (i == BASE_ATTR_HP_REGEN){
                newValue = (getModifiedAttribute(CHAR_ATTR_VITALITY) + 10)
                         * (getModifiedAttribute(CHAR_ATTR_VITALITY) + 10)
                         / (600 / TICKS_PER_HP_REGENERATION);
                         // formula is in HP per minute. 600 game ticks = 1 minute.
            }
            else if (i == BASE_ATTR_HP){
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
                newValue =  0;
                        /* + skill in class of currently equipped weapon ( is
                         * applied during the damage calculation)
                         * weapon attack bonus is applied through equip
                         * modifiers.
                         */
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
    // Inform the client of this attribute modification.
    mModifiedAttributes.insert(attr);
    if (attr == CHAR_ATTR_INTELLIGENCE)
    {
        mSpecialUpdateNeeded = true;
    }
}

int Character::expForLevel(int level)
{
    return int(pow(level, EXPCURVE_EXPONENT) * EXPCURVE_FACTOR);
}

int Character::levelForExp(int exp)
{
    return int(pow(float(exp) / EXPCURVE_FACTOR, 1.0f / EXPCURVE_EXPONENT));
}

void Character::receiveExperience(int skill, int experience, int optimalLevel)
{
    if (skill >= CHAR_ATTR_END)
    {
        // reduce experience when skill is over optimal level
        int levelOverOptimum = getAttribute(skill) - optimalLevel;
        if (optimalLevel && levelOverOptimum > 0)
        {
            experience *= EXP_LEVEL_FLEXIBILITY / (levelOverOptimum + EXP_LEVEL_FLEXIBILITY);
        }

        // add exp
        int oldExp = mExperience[skill];
        long int newExp = mExperience[skill] + experience;
        if (newExp < 0) newExp = 0; // avoid integer underflow/negative exp

        // Check the skill cap
        long int maxSkillCap = Configuration::getValue("maxSkillCap", INT_MAX);
        assert(maxSkillCap <= INT_MAX);  // avoid interger overflow
        if (newExp > maxSkillCap)
        {
            newExp = maxSkillCap;
            if (oldExp != maxSkillCap)
            {
                LOG_INFO("Player hit the skill cap");
                // TODO: send a message to player leting them know they hit the cap
            }
        }
        mExperience[skill] = newExp;
        mModifiedExperience.insert(skill);

        // inform account server
        if (newExp != oldExp)
            accountHandler->updateExperience(getDatabaseID(), skill, newExp);

        // check for skill levelup
        if (Character::levelForExp(newExp) >= Character::levelForExp(oldExp))
        {
            modifiedAttribute(skill);
        }

        mRecalculateLevel = true;
    }
}

void Character::incrementKillCount(int monsterType)
{
    std::map<int, int>::iterator i = mKillCount.find(monsterType);
    if (i == mKillCount.end())
    {
        // character has never murdered this species before
        mKillCount[monsterType] = 1;
    } else {
        // character is a repeated offender
        mKillCount[monsterType] ++;
    };
}

int Character::getKillCount(int monsterType)
{
    std::map<int, int>::iterator i = mKillCount.find(monsterType);
    if (i == mKillCount.end())
    {
        return 0;
    } else {
        return i->second;
    };
}


void Character::recalculateLevel()
{
    std::list<float> levels;
    std::map<int, int>::const_iterator a;
    for (a = getSkillBegin(); a != getSkillEnd(); a++)
    {
        // Only use the first 1000 skill levels in calculation
        if (a->first < 1000)
        {
            float expGot = getExpGot(a->first);
            float expNeed = getExpNeeded(a->first);
            levels.push_back(getAttribute(a->first) + expGot / expNeed);
        }
    }
    levels.sort();

    std::list<float>::iterator i = levels.end();
    float level = 0.0f;
    float factor = 1.0f;
    float factorSum = 0.0f;
    while (i != levels.begin())
    {
        i--;
        level += *i * factor;
        factorSum += factor;
        factor *= LEVEL_SKILL_PRECEDENCE_FACTOR;
    }
    level /= factorSum;
    level += 1.0f; // + 1.0f because the lowest level is 1 and not 0

    while (mLevel < level)
    {
        levelup();
    }

    int levelProgress = int((level - floor(level)) * 100);
    if (levelProgress != mLevelProgress)
    {
        mLevelProgress = levelProgress;
        mUpdateLevelProgress = true;
    }
}

int Character::getExpNeeded(size_t skill)
{
    int level = getAttribute(skill);
    return Character::expForLevel(level + 1) - expForLevel(level);
}

int Character::getExpGot(size_t skill)
{
    int level = getAttribute(skill);
    return mExperience[skill] - Character::expForLevel(level);
}

void Character::levelup()
{
    mLevel++;

    mCharacterPoints += CHARPOINTS_PER_LEVELUP;
    mCorrectionPoints += CORRECTIONPOINTS_PER_LEVELUP;
    if (mCorrectionPoints > CORRECTIONPOINTS_MAX)
        mCorrectionPoints = CORRECTIONPOINTS_MAX;

    MessageOut levelupMsg(GPMSG_LEVELUP);
    levelupMsg.writeShort(mLevel);
    levelupMsg.writeShort(mCharacterPoints);
    levelupMsg.writeShort(mCorrectionPoints);
    gameHandler->sendTo(this, levelupMsg);
    LOG_INFO(getName()<<" reached level "<<mLevel);
}

AttribmodResponseCode Character::useCharacterPoint(size_t attribute)
{
    if (attribute < CHAR_ATTR_BEGIN) return ATTRIBMOD_INVALID_ATTRIBUTE;
    if (attribute >= CHAR_ATTR_END) return ATTRIBMOD_INVALID_ATTRIBUTE;
    if (!mCharacterPoints) return ATTRIBMOD_NO_POINTS_LEFT;

    mCharacterPoints--;
    setAttribute(attribute, getAttribute(attribute) + 1);
    modifiedAttribute(attribute);
    return ATTRIBMOD_OK;
}

AttribmodResponseCode Character::useCorrectionPoint(size_t attribute)
{
    if (attribute < CHAR_ATTR_BEGIN) return ATTRIBMOD_INVALID_ATTRIBUTE;
    if (attribute >= CHAR_ATTR_END) return ATTRIBMOD_INVALID_ATTRIBUTE;
    if (!mCorrectionPoints) return ATTRIBMOD_NO_POINTS_LEFT;
    if (getAttribute(attribute) <= 1) return ATTRIBMOD_DENIED;

    mCorrectionPoints--;
    mCharacterPoints++;
    setAttribute(attribute, getAttribute(attribute) - 1);
    modifiedAttribute(attribute);
    return ATTRIBMOD_OK;
}

void Character::disconnected()
{
    for (Listeners::iterator i = mListeners.begin(),
         i_end = mListeners.end(); i != i_end;)
    {
        const EventListener &l = **i;
        ++i; // In case the listener removes itself from the list on the fly.
        if (l.dispatch->disconnected) l.dispatch->disconnected(&l, this);
    }
}

Character::~Character()
{
    if (getMap())
    {
        Point oldP = getPosition();
        getMap()->getMap()->freeTile(oldP.x / 32, oldP.y / 32, getBlockType());
    }
}

void Character::giveSpecial(int id)
{
    if (mSpecials.find(id) == mSpecials.end())
    {
        // TODO: get the needed mana from a SpecialDB
        int neededMana;
        if (id == 1) neededMana = 10;
        if (id == 2) neededMana = 100;
        if (id == 3) neededMana = 1000;

        Special *s = new Special(neededMana);
        mSpecials[id] = s;
        mSpecialUpdateNeeded = true;
    }
}
