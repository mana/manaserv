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
#include "game-server/attributemanager.hpp"
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

// Experience curve related values
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
    const AttributeScopes &attr =
                           attributeManager->getAttributeInfoForType(ATTR_CHAR);
    LOG_DEBUG("Character creation: initialisation of "
              << attr.size() << " attributes.");
    for (AttributeScopes::const_iterator it1 = attr.begin(),
         it1_end = attr.end(); it1 != it1_end; ++it1)
        mAttributes.insert(std::make_pair(it1->first, Attribute(*it1->second)));

    // Get character data.
    mDatabaseID = msg.readInt32();
    setName(msg.readString());
    deserializeCharacterData(*this, msg);
    mOld = getPosition();
    Inventory(this).initialise();
    modifiedAllAttribute();
    setSize(16);

    // Give the character some specials for testing.
    //TODO: Get from quest vars and equipment
    giveSpecial(1);
    giveSpecial(2);
    giveSpecial(3);
}

void Character::update()
{
    // Update character level
    if (mRecalculateLevel)
    {
        mRecalculateLevel = false;
        recalculateLevel();
    }

    // Update special recharge
    std::list<Special *> rechargeNeeded;
    int numRechargeNeeded = 0;
    for (std::map<int, Special*>::iterator i = mSpecials.begin();
         i != mSpecials.end(); i++)
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
        mRechargePerSpecial = getModifiedAttribute(ATTR_INT)
                              / numRechargeNeeded;
        for (std::list<Special*>::iterator i = rechargeNeeded.begin();
             i != rechargeNeeded.end(); i++)
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

    std::list<AutoAttack> attacks;
    mAutoAttacks.tick(&attacks);
    if (attacks.empty())
        return; // TODO: Install default attack?
    else
        for (std::list<AutoAttack>::iterator it = attacks.begin();
             it != attacks.end(); ++it)
            performAttack(mTarget, it->getDamage());
}

void Character::died()
{
    Being::died();
    Script::executeGlobalEventFunction("on_chr_death", this);
}

void Character::respawn()
{
    if (mAction != DEAD)
    {
        LOG_WARN("Character \"" << getName()
                 << "\" tried to respawn without being dead");
        return;
    }

    // Make it alive again
    setAction(STAND);
    // Reset target
    mTarget = NULL;

    // Execute respawn script
    if (!Script::executeGlobalEventFunction("on_chr_death_accept", this))
    {
        // Script-controlled respawning didn't work - fall back to
        // hardcoded logic.
        mAttributes[ATTR_HP].setBase(mAttributes[ATTR_MAX_HP].getModifiedAttribute());
        updateDerivedAttributes(ATTR_HP);
        // Warp back to spawn point.
        int spawnMap = Configuration::getValue("char_respawnMap", 1);
        int spawnX = Configuration::getValue("char_respawnX", 1024);
        int spawnY = Configuration::getValue("char_respawnY", 1024);
        GameState::enqueueWarp(this, MapManager::getMap(spawnMap), spawnX, spawnY);
    }

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
    Script::performSpecialAction(id, this);
    mSpecialUpdateNeeded = true;
    return;
}

void Character::sendSpecialUpdate()
{
    //GPMSG_SPECIAL_STATUS = 0x0293,
    // { B specialID, L current, L max, L recharge }
    for (std::map<int, Special*>::iterator i = mSpecials.begin();
         i != mSpecials.end(); i++)
    {

        MessageOut msg(GPMSG_SPECIAL_STATUS );
        msg.writeInt8(i->first);
        msg.writeInt32(i->second->currentMana);
        msg.writeInt32(i->second->neededMana);
        msg.writeInt32(mRechargePerSpecial);
        /* Yes, the last one is redundant because it is the same for each
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
        attribMsg.writeInt16(attr);
        attribMsg.writeInt32(getAttribute(attr) * 256);
        attribMsg.writeInt32(getModifiedAttribute(attr) * 256);
    }
    if (attribMsg.getLength() > 2) gameHandler->sendTo(this, attribMsg);
    mModifiedAttributes.clear();

    MessageOut expMsg(GPMSG_PLAYER_EXP_CHANGE);
    for (std::set<size_t>::const_iterator i = mModifiedExperience.begin(),
         i_end = mModifiedExperience.end(); i != i_end; ++i)
    {
        int skill = *i;
        expMsg.writeInt16(skill);
        expMsg.writeInt32(getExpGot(skill));
        expMsg.writeInt32(getExpNeeded(skill));
    }
    if (expMsg.getLength() > 2) gameHandler->sendTo(this, expMsg);
    mModifiedExperience.clear();

    if (mUpdateLevelProgress)
    {
        mUpdateLevelProgress = false;
        MessageOut progressMessage(GPMSG_LEVEL_PROGRESS);
        progressMessage.writeInt8(mLevelProgress);
        gameHandler->sendTo(this, progressMessage);
    }
}

void Character::modifiedAllAttribute()
{
    LOG_DEBUG("Marking all attributes as changed, requiring recalculation.");
    for (AttributeMap::iterator it = mAttributes.begin(),
         it_end = mAttributes.end();
        it != it_end; ++it)
    {
        recalculateBaseAttribute(it->first);
        updateDerivedAttributes(it->first);
    }
}

bool Character::recalculateBaseAttribute(unsigned int attr)
{
    /*
     * `attr' may or may not have changed. Recalculate the base value.
     */
    LOG_DEBUG("Received update attribute recalculation request at Character"
              "for " << attr << ".");
    if (!mAttributes.count(attr))
        return false;
    double newBase = getAttribute(attr);

    /*
     * Calculate new base.
     */
    switch (attr)
    {
    case ATTR_ACCURACY:
        newBase = getModifiedAttribute(ATTR_DEX); // Provisional
        break;
    case ATTR_DEFENSE:
        newBase = 0.3 * getModifiedAttribute(ATTR_VIT);
        break;
    case ATTR_DODGE:
        newBase = getModifiedAttribute(ATTR_AGI); // Provisional
        break;
    case ATTR_MAGIC_DODGE:
        newBase = 1.0;
        // TODO
        break;
    case ATTR_MAGIC_DEFENSE:
        newBase = 0.0;
        // TODO
        break;
    case ATTR_BONUS_ASPD:
        newBase = 0.0;
        // TODO
        break;
    default:
        return Being::recalculateBaseAttribute(attr);
    }

    if (newBase != getAttribute(attr))
    {
        setAttribute(attr, newBase);
        updateDerivedAttributes(attr);
        return true;
    }
    LOG_DEBUG("No changes to sync for attribute '" << attr << "'.");
    return false;
}

void Character::updateDerivedAttributes(unsigned int attr)
{
    /*
     * `attr' has changed, perform updates accordingly.
     */
    flagAttribute(attr);

    switch(attr)
    {
    case ATTR_STR:
        updateDerivedAttributes(ATTR_INV_CAPACITY);
        break;
    case ATTR_AGI:
        updateDerivedAttributes(ATTR_DODGE);
        updateDerivedAttributes(ATTR_MOVE_SPEED_TPS);
        break;
    case ATTR_VIT:
        updateDerivedAttributes(ATTR_MAX_HP);
        updateDerivedAttributes(ATTR_HP_REGEN);
        updateDerivedAttributes(ATTR_DEFENSE);
        break;
    case ATTR_INT:
        // TODO
        break;
    case ATTR_DEX:
        updateDerivedAttributes(ATTR_ACCURACY);
        break;
    case ATTR_WIL:
        // TODO
        break;
    default:
        Being::updateDerivedAttributes(attr);
    }
}

void Character::flagAttribute(int attr)
{
    // Inform the client of this attribute modification.
    accountHandler->updateAttributes(getDatabaseID(), attr,
                                     getAttribute(attr),
                                     getModifiedAttribute(attr));
    mModifiedAttributes.insert(attr);
    if (attr == ATTR_INT)
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
    // reduce experience when skill is over optimal level
    int levelOverOptimum = levelForExp(getExperience(skill)) - optimalLevel;
    if (optimalLevel && levelOverOptimum > 0)
    {
        experience *= EXP_LEVEL_FLEXIBILITY
                      / (levelOverOptimum + EXP_LEVEL_FLEXIBILITY);
    }

    // Add exp
    int oldExp = mExperience[skill];
    long int newExp = mExperience[skill] + experience;
    if (newExp < 0)
        newExp = 0; // Avoid integer underflow/negative exp.

    // Check the skill cap
    long int maxSkillCap = Configuration::getValue("game_maxSkillCap", INT_MAX);
    assert(maxSkillCap <= INT_MAX);  // Avoid integer overflow.
    if (newExp > maxSkillCap)
    {
        newExp = maxSkillCap;
        if (oldExp != maxSkillCap)
        {
            LOG_INFO("Player hit the skill cap");
            // TODO: Send a message to player letting them know they hit the cap
            // or not?
        }
    }
    mExperience[skill] = newExp;
    mModifiedExperience.insert(skill);

    // Inform account server
    if (newExp != oldExp)
        accountHandler->updateExperience(getDatabaseID(), skill, newExp);

    // Check for skill levelup
    if (Character::levelForExp(newExp) >= Character::levelForExp(oldExp))
        updateDerivedAttributes(skill);

    mRecalculateLevel = true;
}

void Character::incrementKillCount(int monsterType)
{
    std::map<int, int>::iterator i = mKillCount.find(monsterType);
    if (i == mKillCount.end())
    {
        // Character has never murdered this species before
        mKillCount[monsterType] = 1;
    }
    else
    {
        // Character is a repeated offender
        mKillCount[monsterType] ++;
    }
}

int Character::getKillCount(int monsterType) const
{
    std::map<int, int>::const_iterator i = mKillCount.find(monsterType);
    if (i != mKillCount.end())
        return i->second;
    return 0;
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
            levels.push_back(levelForExp(a->first) + expGot / expNeed);
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

int Character::getExpNeeded(size_t skill) const
{
    int level = levelForExp(getExperience(skill));
    return Character::expForLevel(level + 1) - expForLevel(level);
}

int Character::getExpGot(size_t skill) const
{
    int level = levelForExp(getExperience(skill));
    return mExperience.at(skill) - Character::expForLevel(level);
}

void Character::levelup()
{
    mLevel++;

    mCharacterPoints += CHARPOINTS_PER_LEVELUP;
    mCorrectionPoints += CORRECTIONPOINTS_PER_LEVELUP;
    if (mCorrectionPoints > CORRECTIONPOINTS_MAX)
        mCorrectionPoints = CORRECTIONPOINTS_MAX;

    MessageOut levelupMsg(GPMSG_LEVELUP);
    levelupMsg.writeInt16(mLevel);
    levelupMsg.writeInt16(mCharacterPoints);
    levelupMsg.writeInt16(mCorrectionPoints);
    gameHandler->sendTo(this, levelupMsg);
    LOG_INFO(getName()<<" reached level "<<mLevel);
}

AttribmodResponseCode Character::useCharacterPoint(size_t attribute)
{
    if (!attributeManager->isAttributeDirectlyModifiable(attribute))
        return ATTRIBMOD_INVALID_ATTRIBUTE;
    if (!mCharacterPoints)
        return ATTRIBMOD_NO_POINTS_LEFT;

    --mCharacterPoints;
    setAttribute(attribute, getAttribute(attribute) + 1);
    updateDerivedAttributes(attribute);
    return ATTRIBMOD_OK;
}

AttribmodResponseCode Character::useCorrectionPoint(size_t attribute)
{
    if (!attributeManager->isAttributeDirectlyModifiable(attribute))
        return ATTRIBMOD_INVALID_ATTRIBUTE;
    if (!mCorrectionPoints)
        return ATTRIBMOD_NO_POINTS_LEFT;
    if (getAttribute(attribute) <= 1)
        return ATTRIBMOD_DENIED;

    --mCorrectionPoints;
    ++mCharacterPoints;
    setAttribute(attribute, getAttribute(attribute) - 1);
    updateDerivedAttributes(attribute);
    return ATTRIBMOD_OK;
}

void Character::disconnected()
{
    for (Listeners::iterator i = mListeners.begin(),
         i_end = mListeners.end(); i != i_end;)
    {
        const EventListener &l = **i;
        ++i; // In case the listener removes itself from the list on the fly.
        if (l.dispatch->disconnected)
            l.dispatch->disconnected(&l, this);
    }
}

Character::~Character()
{
    if (getMap())
    {
        Map *map = getMap()->getMap();
        int tileWidth = map->getTileWidth();
        int tileHeight = map->getTileHeight();
        Point oldP = getPosition();
        map->freeTile(oldP.x / tileWidth, oldP.y / tileHeight, getBlockType());
    }
}

void Character::giveSpecial(int id)
{
    if (mSpecials.find(id) == mSpecials.end())
    {
        Special *s = new Special();
        Script::addDataToSpecial(id, s);
        mSpecials[id] = s;
        mSpecialUpdateNeeded = true;
    }
}

void Character::takeSpecial(int id)
{
    std::map<int, Special*>::iterator i = mSpecials.find(id);
    if (i != mSpecials.end())
    {
        delete i->second;
        mSpecials.erase(i);
        mSpecialUpdateNeeded = true;
    }
}

void Character::clearSpecials()
{
    for(std::map<int, Special*>::iterator i = mSpecials.begin(); i != mSpecials.end(); i++)
    {
        delete i->second;
    }
    mSpecials.clear();
}
