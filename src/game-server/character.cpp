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

#include "game-server/character.h"

#include "common/configuration.h"
#include "game-server/accountconnection.h"
#include "game-server/attributemanager.h"
#include "game-server/buysell.h"
#include "game-server/inventory.h"
#include "game-server/item.h"
#include "game-server/itemmanager.h"
#include "game-server/gamehandler.h"
#include "game-server/map.h"
#include "game-server/mapcomposite.h"
#include "game-server/mapmanager.h"
#include "game-server/skillmanager.h"
#include "game-server/state.h"
#include "game-server/trade.h"
#include "scripting/scriptmanager.h"
#include "net/messagein.h"
#include "net/messageout.h"
#include "serialize/characterdata.h"

#include "utils/logger.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits.h>

// Experience curve related values
const float Character::EXPCURVE_EXPONENT = 3.0f;
const float Character::EXPCURVE_FACTOR = 10.0f;
const float Character::LEVEL_SKILL_PRECEDENCE_FACTOR = 0.75f;
const float Character::EXP_LEVEL_FLEXIBILITY = 1.0f;

Script::Ref Character::mDeathCallback;
Script::Ref Character::mDeathAcceptedCallback;
Script::Ref Character::mLoginCallback;

static bool executeCallback(Script::Ref function, Character *character)
{
    if (!function.isValid())
        return false;

    Script *script = ScriptManager::currentState();
    script->setMap(character->getMap());
    script->prepare(function);
    script->push(character);
    script->execute();
    return true;
}


Character::Character(MessageIn &msg):
    Being(OBJECT_CHARACTER),
    mClient(NULL),
    mConnected(true),
    mTransactionHandler(NULL),
    mSpecialUpdateNeeded(false),
    mDatabaseID(-1),
    mHairStyle(0),
    mHairColor(0),
    mLevel(1),
    mLevelProgress(0),
    mUpdateLevelProgress(false),
    mRecalculateLevel(true),
    mParty(0),
    mTransaction(TRANS_NONE),
    mTalkNpcId(0),
    mNpcThread(0),
    mKnuckleAttackInfo(0)
{
    const AttributeManager::AttributeScope &attr =
                           attributeManager->getAttributeScope(CharacterScope);
    LOG_DEBUG("Character creation: initialisation of "
              << attr.size() << " attributes.");
    for (AttributeManager::AttributeScope::const_iterator it1 = attr.begin(),
         it1_end = attr.end(); it1 != it1_end; ++it1)
        mAttributes.insert(std::make_pair(it1->first, Attribute(*it1->second)));

    setWalkMask(Map::BLOCKMASK_WALL);

    // Get character data.
    mDatabaseID = msg.readInt32();
    setName(msg.readString());
    deserializeCharacterData(*this, msg);
    mOld = getPosition();
    Inventory(this).initialize();
    modifiedAllAttribute();
    setSize(16);

    // Default knuckle attack
    int damageBase = this->getModifiedAttribute(ATTR_STR);
    int damageDelta = damageBase / 2;
    Damage knuckleDamage;
    knuckleDamage.skill = skillManager->getDefaultSkillId();
    knuckleDamage.base = damageBase;
    knuckleDamage.delta = damageDelta;
    knuckleDamage.cth = 2;
    knuckleDamage.element = ELEMENT_NEUTRAL;
    knuckleDamage.type = DAMAGE_PHYSICAL;
    knuckleDamage.range = DEFAULT_TILE_LENGTH;

    mKnuckleAttackInfo = new AttackInfo(0, knuckleDamage, 7, 3, 0);
    addAttack(mKnuckleAttackInfo);
}

Character::~Character()
{
    delete mNpcThread;
    delete mKnuckleAttackInfo;
}

void Character::update()
{
    // First, deal with being generic updates
    Being::update();

    // Update character level if needed.
    if (mRecalculateLevel)
    {
        mRecalculateLevel = false;
        recalculateLevel();
    }

    // Dead character: don't regenerate anything else
    if (getAction() == DEAD)
        return;

    // Update special recharge
    for (SpecialMap::iterator it = mSpecials.begin(), it_end = mSpecials.end();
         it != it_end; it++)
    {
        SpecialValue &s = it->second;
        if (s.specialInfo->rechargeable && s.currentMana < s.specialInfo->neededMana)
        {
            s.currentMana += s.rechargeSpeed;
            if (s.currentMana >= s.specialInfo->neededMana &&
                    s.specialInfo->rechargedCallback.isValid())
            {
                Script *script = ScriptManager::currentState();
                script->prepare(s.specialInfo->rechargedCallback);
                script->push(this);
                script->push(s.specialInfo->id);
                script->execute();
            }
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
}

void Character::died()
{
    Being::died();
    executeCallback(mDeathCallback, this);
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

    // Execute respawn callback when set
    if (executeCallback(mDeathAcceptedCallback, this))
        return;

    // No script respawn callback set - fall back to hardcoded logic
    mAttributes[ATTR_HP].setBase(mAttributes[ATTR_MAX_HP].getModifiedAttribute());
    updateDerivedAttributes(ATTR_HP);
    // Warp back to spawn point.
    int spawnMap = Configuration::getValue("char_respawnMap", 1);
    int spawnX = Configuration::getValue("char_respawnX", 1024);
    int spawnY = Configuration::getValue("char_respawnY", 1024);

    GameState::enqueueWarp(this, MapManager::getMap(spawnMap), spawnX, spawnY);
}

bool Character::specialUseCheck(SpecialMap::iterator it)
{
    if (it == mSpecials.end())
    {
        LOG_INFO("Character uses special " << it->first
                 << " without authorization.");
        return false;
    }

    //check if the special is currently recharged
    SpecialValue &special = it->second;
    if (special.specialInfo->rechargeable &&
            special.currentMana < special.specialInfo->neededMana)
    {
        LOG_INFO("Character uses special " << it->first << " which is not recharged. ("
                 << special.currentMana << "/"
                 << special.specialInfo->neededMana << ")");
        return false;
    }

    if (!special.specialInfo->useCallback.isValid())
    {
        LOG_WARN("No callback for use of special "
                 << special.specialInfo->setName << "/"
                 << special.specialInfo->name << ". Ignoring special.");
        return false;
    }
    return true;
}

void Character::useSpecialOnBeing(int id, Being *b)
{
    SpecialMap::iterator it = mSpecials.find(id);
    if (!specialUseCheck(it))
            return;
    SpecialValue &special = it->second;

    if (special.specialInfo->target != SpecialManager::TARGET_BEING)
        return;

    //tell script engine to cast the spell
    Script *script = ScriptManager::currentState();
    script->setMap(getMap());
    script->prepare(special.specialInfo->useCallback);
    script->push(this);
    script->push(b);
    script->push(special.specialInfo->id);
    script->execute();
}

void Character::useSpecialOnPoint(int id, int x, int y)
{
    SpecialMap::iterator it = mSpecials.find(id);
    if (!specialUseCheck(it))
            return;
    SpecialValue &special = it->second;

    if (special.specialInfo->target != SpecialManager::TARGET_POINT)
        return;

    //tell script engine to cast the spell
    Script *script = ScriptManager::currentState();
    script->setMap(getMap());
    script->prepare(special.specialInfo->useCallback);
    script->push(this);
    script->push(x);
    script->push(y);
    script->push(special.specialInfo->id);
    script->execute();
}

bool Character::giveSpecial(int id, int currentMana)
{
    if (mSpecials.find(id) == mSpecials.end())
    {
        const SpecialManager::SpecialInfo *specialInfo =
                specialManager->getSpecialInfo(id);
        if (!specialInfo)
        {
            LOG_ERROR("Tried to give not existing special id " << id << ".");
            return false;
        }
        mSpecials.insert(std::pair<int, SpecialValue>(
                             id, SpecialValue(currentMana, specialInfo)));
        mSpecialUpdateNeeded = true;
        return true;
    }
    return false;
}

bool Character::setSpecialMana(int id, int mana)
{
    SpecialMap::iterator it = mSpecials.find(id);
    if (it != mSpecials.end())
    {
        it->second.currentMana = mana;
        mSpecialUpdateNeeded = true;
        return true;
    }
    return false;
}

bool Character::setSpecialRechargeSpeed(int id, int speed)
{
    SpecialMap::iterator it = mSpecials.find(id);
    if (it != mSpecials.end())
    {
        it->second.rechargeSpeed = speed;
        mSpecialUpdateNeeded = true;
        return true;
    }
    return false;
}

void Character::sendSpecialUpdate()
{
    //GPMSG_SPECIAL_STATUS = 0x0293,
    // { B specialID, L current, L max, L recharge }

    MessageOut msg(GPMSG_SPECIAL_STATUS);
    for (SpecialMap::iterator it = mSpecials.begin(), it_end = mSpecials.end();
         it != it_end; ++it)
    {
        msg.writeInt8(it->first);
        msg.writeInt32(it->second.currentMana);
        msg.writeInt32(it->second.specialInfo->neededMana);
        msg.writeInt32(it->second.rechargeSpeed);
    }
    gameHandler->sendTo(this, msg);
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
        expMsg.writeInt16(levelForExp(getExperience(skill)));
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

void Character::recalculateBaseAttribute(unsigned attr)
{
    // `attr' may or may not have changed. Recalculate the base value.
    LOG_DEBUG("Received update attribute recalculation request at Character "
              "for " << attr << ".");
    if (!mAttributes.count(attr))
        return;

    if (attr == ATTR_STR && mKnuckleAttackInfo)
    {
        // TODO: dehardcode this
        Damage &knuckleDamage = mKnuckleAttackInfo->getDamage();
        knuckleDamage.base = getModifiedAttribute(ATTR_STR);
        knuckleDamage.delta = knuckleDamage.base / 2;
    }
    Being::recalculateBaseAttribute(attr);

}


void Character::updateDerivedAttributes(unsigned attr)
{
    /*
     * `attr' has changed, perform updates accordingly.
     */
    flagAttribute(attr);


    Being::updateDerivedAttributes(attr);
}

void Character::flagAttribute(int attr)
{
    // Inform the client of this attribute modification.
    accountHandler->updateAttributes(getDatabaseID(), attr,
                                     getAttribute(attr),
                                     getModifiedAttribute(attr));
    mModifiedAttributes.insert(attr);
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

void Character::startNpcThread(Script::Thread *thread, int npcId)
{
    mNpcThread = thread;
    mTalkNpcId = npcId;

    resumeNpcThread();
}

void Character::resumeNpcThread()
{
    Script *script = ScriptManager::currentState();

    assert(script->getCurrentThread() == mNpcThread);

    if (script->resume())
    {
        MessageOut msg(GPMSG_NPC_CLOSE);
        msg.writeInt16(mTalkNpcId);
        gameHandler->sendTo(this, msg);

        mTalkNpcId = 0;
        mNpcThread = 0;
    }
}

void Character::addAttack(AttackInfo *attackInfo)
{
    // Remove knuckle attack
    Being::addAttack(attackInfo);
    Being::removeAttack(mKnuckleAttackInfo);
}

void Character::removeAttack(AttackInfo *attackInfo)
{
    Being::removeAttack(attackInfo);
    // Add knuckle attack
    if (mAttacks.getNumber() == 0)
        Being::addAttack(mKnuckleAttackInfo);
}

void Character::disconnected()
{
    mConnected = false;

    // Make the dead characters respawn, even in case of disconnection.
    if (getAction() == DEAD)
        respawn();
    else
        GameState::remove(this);

    signal_disconnected.emit(this);
}

bool Character::takeSpecial(int id)
{
    SpecialMap::iterator i = mSpecials.find(id);
    if (i != mSpecials.end())
    {
        mSpecials.erase(i);
        mSpecialUpdateNeeded = true;
        return true;
    }
    return false;
}

void Character::clearSpecials()
{
    mSpecials.clear();
}

void Character::triggerLoginCallback()
{
    executeCallback(mLoginCallback, this);
}
