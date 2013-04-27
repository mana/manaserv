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
#include "game-server/attack.h"
#include "game-server/attributemanager.h"
#include "game-server/buysell.h"
#include "game-server/combatcomponent.h"
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
const float CharacterComponent::EXPCURVE_EXPONENT = 2.5f;
const float CharacterComponent::EXPCURVE_FACTOR = 20.0f;
const float CharacterComponent::LEVEL_SKILL_PRECEDENCE_FACTOR = 0.85f;
const float CharacterComponent::EXP_LEVEL_FLEXIBILITY = 3.0f;

Script::Ref CharacterComponent::mDeathCallback;
Script::Ref CharacterComponent::mDeathAcceptedCallback;
Script::Ref CharacterComponent::mLoginCallback;

static bool executeCallback(Script::Ref function, Entity &entity)
{
    if (!function.isValid())
        return false;

    Script *script = ScriptManager::currentState();
    script->prepare(function);
    script->push(&entity);
    script->execute(entity.getMap());
    return true;
}


CharacterComponent::CharacterComponent(Entity &entity, MessageIn &msg):
    mClient(nullptr),
    mConnected(true),
    mTransactionHandler(nullptr),
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
    mKnuckleAttackInfo(0),
    mBaseEntity(&entity)
{
    auto *beingComponent = entity.getComponent<BeingComponent>();

    const AttributeManager::AttributeScope &attributes =
                           attributeManager->getAttributeScope(CharacterScope);
    LOG_DEBUG("Character creation: initialisation of "
              << attributes.size() << " attributes.");
    for (auto attributeScope : attributes)
        beingComponent->createAttribute(attributeScope.first,
                                        *attributeScope.second);

    auto *actorComponent = entity.getComponent<ActorComponent>();
    actorComponent->setWalkMask(Map::BLOCKMASK_WALL);
    actorComponent->setBlockType(BLOCKTYPE_CHARACTER);
    actorComponent->setSize(16);


    CombatComponent *combatcomponent = new CombatComponent(entity);
    entity.addComponent(combatcomponent);
    combatcomponent->getAttacks().attack_added.connect(
            sigc::mem_fun(this, &CharacterComponent::attackAdded));
    combatcomponent->getAttacks().attack_removed.connect(
            sigc::mem_fun(this, &CharacterComponent::attackRemoved));

    // Default knuckle attack
    int damageBase = beingComponent->getModifiedAttribute(ATTR_STR);
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
    combatcomponent->addAttack(mKnuckleAttackInfo);

    // Get character data.
    mDatabaseID = msg.readInt32();
    beingComponent->setName(msg.readString());

    CharacterData characterData(&entity, this);
    deserializeCharacterData(characterData, msg);

    Inventory(&entity, mPossessions).initialize();
    modifiedAllAttributes(entity);;

    beingComponent->signal_attribute_changed.connect(sigc::mem_fun(
            this, &CharacterComponent::attributeChanged));
}

CharacterComponent::~CharacterComponent()
{
    delete mNpcThread;
    delete mKnuckleAttackInfo;
}

void CharacterComponent::update(Entity &entity)
{
    // Update character level if needed.
    if (mRecalculateLevel)
    {
        mRecalculateLevel = false;
        recalculateLevel(entity);
    }

    // Dead character: don't regenerate anything else
    if (entity.getComponent<BeingComponent>()->getAction() == DEAD)
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
                script->push(&entity);
                script->push(s.specialInfo->id);
                script->execute(entity.getMap());
            }
        }
    }

    if (mSpecialUpdateNeeded)
    {
        sendSpecialUpdate();
        mSpecialUpdateNeeded = false;
    }
}

void CharacterComponent::characterDied(Entity *being)
{
    executeCallback(mDeathCallback, *being);
}

void CharacterComponent::respawn(Entity &entity)
{
    auto *beingComponent = entity.getComponent<BeingComponent>();

    if (beingComponent->getAction() != DEAD)
    {
        LOG_WARN("Character \"" << beingComponent->getName()
                 << "\" tried to respawn without being dead");
        return;
    }

    // Make it alive again
    beingComponent->setAction(entity, STAND);
    // Reset target
    entity.getComponent<CombatComponent>()->clearTarget();

    // Execute respawn callback when set
    if (executeCallback(mDeathAcceptedCallback, entity))
        return;

    // No script respawn callback set - fall back to hardcoded logic
    const double maxHp = beingComponent->getModifiedAttribute(ATTR_MAX_HP);
    beingComponent->setAttribute(entity, ATTR_HP, maxHp);
    // Warp back to spawn point.
    int spawnMap = Configuration::getValue("char_respawnMap", 1);
    int spawnX = Configuration::getValue("char_respawnX", 1024);
    int spawnY = Configuration::getValue("char_respawnY", 1024);

    GameState::enqueueWarp(&entity, MapManager::getMap(spawnMap),
                           Point(spawnX, spawnY));
}

bool CharacterComponent::specialUseCheck(SpecialMap::iterator it)
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

void CharacterComponent::useSpecialOnBeing(Entity &user, int id, Entity *b)
{
    SpecialMap::iterator it = mSpecials.find(id);
    if (!specialUseCheck(it))
            return;
    SpecialValue &special = it->second;

    if (special.specialInfo->target != SpecialManager::TARGET_BEING)
        return;

    //tell script engine to cast the spell
    Script *script = ScriptManager::currentState();
    script->prepare(special.specialInfo->useCallback);
    script->push(&user);
    script->push(b);
    script->push(special.specialInfo->id);
    script->execute(user.getMap());
}

void CharacterComponent::useSpecialOnPoint(Entity &user, int id, int x, int y)
{
    SpecialMap::iterator it = mSpecials.find(id);
    if (!specialUseCheck(it))
            return;
    SpecialValue &special = it->second;

    if (special.specialInfo->target != SpecialManager::TARGET_POINT)
        return;

    //tell script engine to cast the spell
    Script *script = ScriptManager::currentState();
    script->prepare(special.specialInfo->useCallback);
    script->push(&user);
    script->push(x);
    script->push(y);
    script->push(special.specialInfo->id);
    script->execute(user.getMap());
}

bool CharacterComponent::giveSpecial(int id, int currentMana)
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

bool CharacterComponent::setSpecialMana(int id, int mana)
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

bool CharacterComponent::setSpecialRechargeSpeed(int id, int speed)
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

void CharacterComponent::sendSpecialUpdate()
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
    gameHandler->sendTo(mClient, msg);
}

void CharacterComponent::cancelTransaction()
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

Trade *CharacterComponent::getTrading() const
{
    return mTransaction == TRANS_TRADE
        ? static_cast< Trade * >(mTransactionHandler) : nullptr;
}

BuySell *CharacterComponent::getBuySell() const
{
    return mTransaction == TRANS_BUYSELL
        ? static_cast< BuySell * >(mTransactionHandler) : nullptr;
}

void CharacterComponent::setTrading(Trade *t)
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

void CharacterComponent::setBuySell(BuySell *t)
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

void CharacterComponent::sendStatus(Entity &entity)
{
    auto *beingComponent = entity.getComponent<BeingComponent>();
    MessageOut attribMsg(GPMSG_PLAYER_ATTRIBUTE_CHANGE);
    for (std::set<size_t>::const_iterator i = mModifiedAttributes.begin(),
         i_end = mModifiedAttributes.end(); i != i_end; ++i)
    {
        int attr = *i;
        attribMsg.writeInt16(attr);
        attribMsg.writeInt32(beingComponent->getAttributeBase(attr) * 256);
        attribMsg.writeInt32(beingComponent->getModifiedAttribute(attr) * 256);
    }
    if (attribMsg.getLength() > 2) gameHandler->sendTo(mClient, attribMsg);
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
    if (expMsg.getLength() > 2) gameHandler->sendTo(mClient, expMsg);
    mModifiedExperience.clear();

    if (mUpdateLevelProgress)
    {
        mUpdateLevelProgress = false;
        MessageOut progressMessage(GPMSG_LEVEL_PROGRESS);
        progressMessage.writeInt8(mLevelProgress);
        gameHandler->sendTo(mClient, progressMessage);
    }
}

void CharacterComponent::modifiedAllAttributes(Entity &entity)
{
    auto *beingComponent = entity.getComponent<BeingComponent>();

    LOG_DEBUG("Marking all attributes as changed, requiring recalculation.");
    for (auto attribute : beingComponent->getAttributes())
    {
        beingComponent->recalculateBaseAttribute(entity, attribute.first);
        mModifiedAttributes.insert(attribute.first);
    }
}

void CharacterComponent::attributeChanged(Entity *entity, unsigned attr)
{
    auto *beingComponent = entity->getComponent<BeingComponent>();

    // Inform the client of this attribute modification.
    accountHandler->updateAttributes(getDatabaseID(), attr,
                                   beingComponent->getAttributeBase(attr),
                                   beingComponent->getModifiedAttribute(attr));
    mModifiedAttributes.insert(attr);

    // Update the knuckle Attack if required
    if (attr == ATTR_STR && mKnuckleAttackInfo)
    {
        // TODO: dehardcode this
        Damage &knuckleDamage = mKnuckleAttackInfo->getDamage();
        knuckleDamage.base = beingComponent->getModifiedAttribute(ATTR_STR);
        knuckleDamage.delta = knuckleDamage.base / 2;
    }
}

int CharacterComponent::expForLevel(int level)
{
    return int(pow(level, EXPCURVE_EXPONENT) * EXPCURVE_FACTOR);
}

int CharacterComponent::levelForExp(int exp)
{
    return int(pow(float(exp) / EXPCURVE_FACTOR, 1.0f / EXPCURVE_EXPONENT));
}

void CharacterComponent::receiveExperience(int skill, int experience, int optimalLevel)
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

void CharacterComponent::incrementKillCount(int monsterType)
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

int CharacterComponent::getKillCount(int monsterType) const
{
    std::map<int, int>::const_iterator i = mKillCount.find(monsterType);
    if (i != mKillCount.end())
        return i->second;
    return 0;
}


void CharacterComponent::recalculateLevel(Entity &entity)
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
        levelup(entity);
    }

    int levelProgress = int((level - floor(level)) * 100);
    if (levelProgress != mLevelProgress)
    {
        mLevelProgress = levelProgress;
        mUpdateLevelProgress = true;
    }
}

int CharacterComponent::getExpNeeded(size_t skill) const
{
    int level = levelForExp(getExperience(skill));
    return CharacterComponent::expForLevel(level + 1) - expForLevel(level);
}

int CharacterComponent::getExpGot(size_t skill) const
{
    int level = levelForExp(getExperience(skill));
    return mExperience.at(skill) - CharacterComponent::expForLevel(level);
}

void CharacterComponent::levelup(Entity &entity)
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
    gameHandler->sendTo(mClient, levelupMsg);
    LOG_INFO(entity.getComponent<BeingComponent>()->getName()
             << " reached level " << mLevel);
}

AttribmodResponseCode CharacterComponent::useCharacterPoint(Entity &entity,
                                                            size_t attribute)
{
    auto *beingComponent = entity.getComponent<BeingComponent>();

    if (!attributeManager->isAttributeDirectlyModifiable(attribute))
        return ATTRIBMOD_INVALID_ATTRIBUTE;
    if (!mCharacterPoints)
        return ATTRIBMOD_NO_POINTS_LEFT;

    --mCharacterPoints;

    const double base = beingComponent->getAttributeBase(attribute);
    beingComponent->setAttribute(entity, attribute, base + 1);
    beingComponent->updateDerivedAttributes(entity, attribute);
    return ATTRIBMOD_OK;
}

AttribmodResponseCode CharacterComponent::useCorrectionPoint(Entity &entity,
                                                             size_t attribute)
{
    auto *beingComponent = entity.getComponent<BeingComponent>();

    if (!attributeManager->isAttributeDirectlyModifiable(attribute))
        return ATTRIBMOD_INVALID_ATTRIBUTE;
    if (!mCorrectionPoints)
        return ATTRIBMOD_NO_POINTS_LEFT;
    if (beingComponent->getAttributeBase(attribute) <= 1)
        return ATTRIBMOD_DENIED;

    --mCorrectionPoints;
    ++mCharacterPoints;

    const double base = beingComponent->getAttributeBase(attribute);
    beingComponent->setAttribute(entity, attribute, base - 1);
    return ATTRIBMOD_OK;
}

void CharacterComponent::startNpcThread(Script::Thread *thread, int npcId)
{
    if (mNpcThread)
        delete mNpcThread;

    mNpcThread = thread;
    mTalkNpcId = npcId;

    resumeNpcThread();
}

void CharacterComponent::resumeNpcThread()
{
    Script *script = ScriptManager::currentState();

    assert(script->getCurrentThread() == mNpcThread);

    if (script->resume())
    {
        MessageOut msg(GPMSG_NPC_CLOSE);
        msg.writeInt16(mTalkNpcId);
        gameHandler->sendTo(mClient, msg);

        mTalkNpcId = 0;
        mNpcThread = 0;
    }
}

void CharacterComponent::attackAdded(CombatComponent *combatComponent,
                                     Attack &attack)
{
    // Remove knuckle attack
    if (attack.getAttackInfo() != mKnuckleAttackInfo)
        combatComponent->removeAttack(mKnuckleAttackInfo);
}

void CharacterComponent::attackRemoved(CombatComponent *combatComponent,
                                       Attack &attack)
{
    // Add knuckle attack
    // 1 since the attack is not really removed yet.
    if (combatComponent->getAttacks().getNumber() == 1)
        combatComponent->addAttack(mKnuckleAttackInfo);
}

void CharacterComponent::disconnected(Entity &entity)
{
    mConnected = false;

    // Make the dead characters respawn, even in case of disconnection.
    if (entity.getComponent<BeingComponent>()->getAction() == DEAD)
        respawn(entity);
    else
        GameState::remove(&entity);

    signal_disconnected.emit(entity);
}

bool CharacterComponent::takeSpecial(int id)
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

void CharacterComponent::clearSpecials()
{
    mSpecials.clear();
}

void CharacterComponent::triggerLoginCallback(Entity &entity)
{
    executeCallback(mLoginCallback, entity);
}
