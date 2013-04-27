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

#ifndef CHARACTER_H
#define CHARACTER_H

#include "common/defines.h"
#include "common/inventorydata.h"
#include "common/manaserv_protocol.h"

#include "game-server/abilitycomponent.h"
#include "game-server/being.h"
#include "game-server/mapcomposite.h"
#include "game-server/mapmanager.h"
#include "game-server/abilitymanager.h"

#include "scripting/script.h"

#include "utils/logger.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class BuySell;
class GameClient;
class MessageIn;
class MessageOut;
class Point;
class Trade;


class CharacterData
{
public:
    CharacterData(Entity *entity, CharacterComponent *characterComponent);

    void setGender(BeingGender);
    BeingGender getGender() const;

    void setMapId(int id);
    int getMapId() const;
    void setPosition(Point &point);
    const Point &getPosition() const;

    void setAttribute(int id, int base);
    void setModAttribute(int id, int mod);
    const AttributeMap &getAttributes() const;
    void setCharacterPoints(int characterPoints);
    int getCharacterPoints() const;
    void setCorrectionPoints(int correctionPoints);
    int getCorrectionPoints() const;

    void setExperience(int skill, int level);
    void setLevel(int level) const;
    int getLevel() const;

    int getAccountLevel() const;

    void setHairStyle(int style);
    int getHairStyle() const;
    void setHairColor(int color);
    int getHairColor() const;

    void setAccountLevel(int level);

    int getSkillSize() const;
    const std::map<int, int>::const_iterator getSkillBegin() const;
    const std::map<int, int>::const_iterator getSkillEnd() const;

    void applyStatusEffect(int status, int time);
    int getStatusEffectSize() const;
    const std::map<int, Status>::const_iterator getStatusEffectBegin() const;
    const std::map<int, Status>::const_iterator getStatusEffectEnd() const;

    int getKillCountSize() const;
    const std::map<int, int>::const_iterator getKillCountBegin() const;
    const std::map<int, int>::const_iterator getKillCountEnd() const;
    void setKillCount(int monsterId, int kills);

    void clearAbilities();
    void giveAbility(int id, int mana);
    int getAbilitySize() const;
    AbilityMap::const_iterator getAbilityBegin() const;
    AbilityMap::const_iterator getAbilityEnd() const;

    Possessions &getPossessions() const;

private:
    Entity *mEntity;
    CharacterComponent *mCharacterComponent;
};


/**
 * The representation of a player's character in the game world.
 */
class CharacterComponent : public Component
{
    public:
        static const ComponentType type = CT_Character;

        /**
         * Utility constructor for creating a Character from a received
         * characterdata message.
         */
        CharacterComponent(Entity &entity, MessageIn &msg);

        ~CharacterComponent();

        /**
         * recalculates the level when necessary and calls Being::update
         */
        void update(Entity &entity);

        /**
         * Executes the global die script
         */
        virtual void characterDied(Entity *);

        /**
         * makes the character respawn
         */
        void respawn(Entity &entity);

        /**
         * Gets client computer.
         */
        GameClient *getClient() const
        { return mClient; }

        /**
         * Sets client computer.
         */
        void setClient(GameClient *c)
        { mClient = c; }

        /**
         * Gets a reference to the possessions.
         */
        const Possessions &getPossessions() const
        { return mPossessions; }

        /**
         * Gets a reference to the possessions.
         */
        Possessions &getPossessions()
        { return mPossessions; }

        /**
         * Gets the Trade object the character is involved in.
         */
        Trade *getTrading() const;

        /**
         * Sets the Trade object the character is involved in.
         * Cancels other transactions.
         */
        void setTrading(Trade *t);

        /**
         * Gets the BuySell object the character is involved in.
         */
        BuySell *getBuySell() const;

        /**
         * Sets the trade object the character is involved in.
         * Cancels other transactions.
         */
        void setBuySell(BuySell *t);

        /**
         * Cancels current transaction.
         */
        void cancelTransaction();

        /**
         * Gets transaction status of the character.
         */
        bool isBusy() const
        { return mTransaction != TRANS_NONE; }

        /*
         * Character data:
         * Get and set methods
         * Most of this should be accessed directly as a friend
         */

        int getDatabaseID() const { return mDatabaseID; }
        void setDatabaseID(int id) { mDatabaseID = id; }

        int getHairStyle() const { return mHairStyle; }
        void setHairStyle(int style) { mHairStyle = style; }

        int getHairColor() const { return mHairColor; }
        void setHairColor(int color) { mHairColor = color; }

        int getLevel() const { return mLevel; }
        void setLevel(int level) { mLevel = level; }

        int getAccountLevel() const { return mAccountLevel; }
        void setAccountLevel(int l) { mAccountLevel = l; }

        /** Gets the party id of the character */
        int getParty() const
        { return mParty; }

        /** Sets the party id of the character */
        void setParty(int party)
        { mParty = party; }

        /**
         * Sends a message that informs the client about attribute
         * modified since last call.
         */
        void sendStatus(Entity &entity);

        /**
         * Marks all attributes as being modified.
         */
        void modifiedAllAttributes(Entity &entity);

        /**
         * Signal handler for attribute changed event
         * Flags the attribute as modified.
         * @param being th being of which the attribute was changed
         * @param attributeId the changed id
         */
        void attributeChanged(Entity *being, unsigned attributeId);

        /**
         * Calls all the "disconnected" listener.
         */
        void disconnected(Entity &entity);

        /**
         * Associative array containing all the quest variables known by the
         * server.
         */
        std::map< std::string, std::string > questCache;

        /**
         * Gives a skill a specific amount of exp and checks if a levelup
         * occured.
         */
        void receiveExperience(int skill, int experience, int optimalLevel);

        int getSkillSize() const
        { return mExperience.size(); }

        const std::map<int, int>::const_iterator getSkillBegin() const
        { return mExperience.begin(); }

        const std::map<int, int>::const_iterator getSkillEnd() const
        { return mExperience.end(); }

        /**
         * Used to serialize kill count.
         */
        int getKillCountSize() const
        { return mKillCount.size(); }

        const std::map<int, int>::const_iterator getKillCountBegin() const
        { return mKillCount.begin(); }

        const std::map<int, int>::const_iterator getKillCountEnd() const
        { return mKillCount.end(); }

        void setKillCount(int monsterId, int kills)
        { mKillCount[monsterId] = kills; }

        /**
         * Gets total accumulated exp for skill.
         */
        int getExperience(int skill) const
        { return mExperience.find(skill)->second; }

        /**
         * Sets total accumulated exp for skill.
         */
        void setExperience(int skill, int value)
        { mExperience[skill] = 0; receiveExperience(skill, value, 0); }

        /**
         * Adds one kill of the monster type to the characters kill count.
         */
        void incrementKillCount(int monsterType);

        /**
         * Gets the number of monsters the character killed of a given type.
         */
        int getKillCount(int monsterType) const;

        /**
         * Returns the exp needed to reach a specific skill level
         */
        static int expForLevel(int level);

        /**
         * Returns the level for a given exp
         */
        static int levelForExp(int exp);

        /**
         * Tries to use a character point to increase a
         * basic attribute
         */
        AttribmodResponseCode useCharacterPoint(Entity &entity,
                                                size_t attribute);

        /**
         * Tries to use a correction point to reduce a
         * basic attribute and regain a character point
         */
        AttribmodResponseCode useCorrectionPoint(Entity &entity,
                                                 size_t attribute);

        void setCharacterPoints(int points) { mCharacterPoints = points; }
        int getCharacterPoints() const { return mCharacterPoints; }

        void setCorrectionPoints(int points) { mCorrectionPoints = points; }
        int getCorrectionPoints() const { return mCorrectionPoints; }


        /**
         * Starts the given NPC thread.
         *
         * Should be called immediately after creating the thread and pushing
         * the NPC function and its parameters.
         */
        void startNpcThread(Script::Thread *thread, int npcId);

        /**
         * Resumes the given NPC thread of this character and sends the NPC
         * close message to the player when the script is done.
         *
         * Should be called after preparing the current Script instance for
         * resuming the thread and pushing the parameters the script expects.
         */
        void resumeNpcThread();

        /**
         * Returns the NPC thread in use by this character, if any.
         */
        Script::Thread *getNpcThread() const
        { return mNpcThread; }

        /** Makes it impossible to chat for a while */
        void mute(int seconds)
        { mMuteTimeout.set(seconds * 10); }

        bool isMuted() const
        { return !mMuteTimeout.expired(); }

        bool isConnected() const
        { return mConnected; }

        static void setDeathCallback(Script *script)
        { script->assignCallback(mDeathCallback); }

        static void setDeathAcceptedCallback(Script *script)
        { script->assignCallback(mDeathAcceptedCallback); }

        static void setLoginCallback(Script *script)
        { script->assignCallback(mLoginCallback); }

        void triggerLoginCallback(Entity &entity);

        sigc::signal<void, Entity &> signal_disconnected;

    private:
        double getAttrBase(AttributeMap::const_iterator it) const
        { return it->second.getBase(); }
        double getAttrMod(AttributeMap::const_iterator it) const
        { return it->second.getModifiedAttribute(); }

        CharacterComponent(const CharacterComponent &);
        CharacterComponent &operator=(const CharacterComponent &);

        static const float EXPCURVE_EXPONENT;
        static const float EXPCURVE_FACTOR;
        static const float LEVEL_SKILL_PRECEDENCE_FACTOR; // I am taking suggestions for a better name
        static const float EXP_LEVEL_FLEXIBILITY;
        static const int CHARPOINTS_PER_LEVELUP = 5;
        static const int CORRECTIONPOINTS_PER_LEVELUP = 2;
        static const int CORRECTIONPOINTS_MAX = 10;

        /**
         * Advances the character by one level;
         */
        void levelup(Entity &entity);

        /**
         * Returns the exp needed for next skill levelup
         */
        int getExpNeeded(size_t skill) const;

        /**
         * Returns the exp collected on this skill level
         */
        int getExpGot(size_t skill) const;

        /**
         * Recalculates the character level
         */
        void recalculateLevel(Entity &entity);

        void abilityStatusChanged(int id);
        void abilityCooldownActivated();

        /**
         * Informs the client about his characters abilities charge status
         */
        void sendAbilityUpdate(Entity &entity);

        void sendAbilityCooldownUpdate(Entity &entity);

        enum TransactionType
        { TRANS_NONE, TRANS_TRADE, TRANS_BUYSELL };

        GameClient *mClient;   /**< Client computer. */

        /**
         * Tells whether the character client is connected.
         * Useful when dealing with enqueued events.
         */
        bool mConnected;

        /** Handler of the transaction the character is involved in. */
        void *mTransactionHandler;

        Possessions mPossessions;    /**< Possesssions of the character. */

        /** Attributes modified since last update. */
        std::set<size_t> mModifiedAttributes;
        std::set<size_t> mModifiedExperience;

        std::map<int, int> mExperience; /**< experience collected for each skill.*/

        std::set<unsigned> mModifiedAbilities;

        int mDatabaseID;             /**< Character's database ID. */
        unsigned char mHairStyle;    /**< Hair Style of the character. */
        unsigned char mHairColor;    /**< Hair Color of the character. */
        int mLevel;                  /**< Level of the character. */
        int mLevelProgress;          /**< progress to next level in percent */
        int mCharacterPoints;        /**< Unused attribute points that can be distributed */
        int mCorrectionPoints;       /**< Unused attribute correction points */
        bool mUpdateLevelProgress;   /**< Flag raised when percent to next level changed */
        bool mRecalculateLevel;      /**< Flag raised when the character level might have increased */
        bool mSendAbilityCooldown;
        unsigned char mAccountLevel; /**< Account level of the user. */
        int mParty;                  /**< Party id of the character */
        TransactionType mTransaction; /**< Trade/buy/sell action the character is involved in. */
        std::map<int, int> mKillCount;  /**< How many monsters the character has slain of each type */

        int mTalkNpcId;              /**< Public ID of NPC the character is talking to, if any */
        Script::Thread *mNpcThread;  /**< Script thread executing NPC interaction, if any */

        Timeout mMuteTimeout;        /**< Time until the character is no longer muted  */

        Entity *mBaseEntity;        /**< The entity this component is part of
                                         this is ONLY required to allow using
                                         the serialization routine without many
                                         changes (we cannot pass the entity as
                                         argument there). DO NOT USE THIS IF IT
                                         IS AVOIDABLE in order to allow
                                         refactoring this easier later! */

        static Script::Ref mDeathCallback;
        static Script::Ref mDeathAcceptedCallback;
        static Script::Ref mLoginCallback;
};


inline CharacterData::CharacterData(Entity *entity,
                                    CharacterComponent *characterComponent):
        mEntity(entity),
        mCharacterComponent(characterComponent)
{}

inline void CharacterData::setGender(BeingGender gender)
{
    mEntity->getComponent<BeingComponent>()->setGender(gender);
}

inline BeingGender CharacterData::getGender() const
{
    return mEntity->getComponent<BeingComponent>()->getGender();
}

inline void CharacterData::setMapId(int id)
{
    mEntity->setMap(MapManager::getMap(id));
}

inline int CharacterData::getMapId() const
{
    return mEntity->getMap()->getID();
}

inline void CharacterData::setPosition(Point &point)
{
    mEntity->getComponent<ActorComponent>()->setPosition(*mEntity, point);
}

inline const Point &CharacterData::getPosition() const
{
    return mEntity->getComponent<ActorComponent>()->getPosition();
}

inline void CharacterData::setAttribute(int id, int base)
{
    mEntity->getComponent<BeingComponent>()->setAttribute(*mEntity, id, base);
}

inline void CharacterData::setModAttribute(int id, int mod)
{
    mEntity->getComponent<BeingComponent>()->setModAttribute(id, mod);
}

inline const AttributeMap &CharacterData::getAttributes() const
{
    return mEntity->getComponent<BeingComponent>()->getAttributes();
}

inline void CharacterData::setCharacterPoints(int characterPoints)
{
    mCharacterComponent->setCharacterPoints(characterPoints);
}

inline int CharacterData::getCharacterPoints() const
{
    return mCharacterComponent->getCharacterPoints();
}

inline void CharacterData::setCorrectionPoints(int correctionPoints)
{
    mCharacterComponent->setCorrectionPoints(correctionPoints);
}

inline int CharacterData::getCorrectionPoints() const
{
    return mCharacterComponent->getCorrectionPoints();
}

inline void CharacterData::setExperience(int skill, int level)
{
    mCharacterComponent->setExperience(skill, level);
}

inline void CharacterData::setLevel(int level) const
{
    mCharacterComponent->setLevel(level);
}

inline int CharacterData::getLevel() const
{
    return mCharacterComponent->getLevel();
}

inline int CharacterData::getAccountLevel() const
{
    return mCharacterComponent->getAccountLevel();
}

inline void CharacterData::setHairStyle(int style)
{
    mCharacterComponent->setHairStyle(style);
}

inline int CharacterData::getHairStyle() const
{
    return mCharacterComponent->getHairStyle();
}

inline void CharacterData::setHairColor(int color)
{
    mCharacterComponent->setHairColor(color);
}

inline int CharacterData::getHairColor() const
{
    return mCharacterComponent->getHairColor();
}

inline void CharacterData::setAccountLevel(int level)
{
    mCharacterComponent->setAccountLevel(level);
}

inline int CharacterData::getSkillSize() const
{
    return mCharacterComponent->getSkillSize();
}

inline const std::map<int, int>::const_iterator CharacterData::getSkillBegin() const
{
    return mCharacterComponent->getSkillBegin();
}

inline const std::map<int, int>::const_iterator CharacterData::getSkillEnd() const
{
    return mCharacterComponent->getSkillEnd();
}

inline void CharacterData::applyStatusEffect(int status, int time)
{
    mEntity->getComponent<BeingComponent>()->applyStatusEffect(status, time);
}

inline int CharacterData::getStatusEffectSize() const
{
    return mEntity->getComponent<BeingComponent>()->getStatusEffects().size();
}

inline const std::map<int, Status>::const_iterator CharacterData::getStatusEffectBegin() const
{
    return mEntity->getComponent<BeingComponent>()->getStatusEffects().begin();
}

inline const std::map<int, Status>::const_iterator CharacterData::getStatusEffectEnd() const
{
    return mEntity->getComponent<BeingComponent>()->getStatusEffects().end();
}

inline int CharacterData::getKillCountSize() const
{
    return mCharacterComponent->getKillCountSize();
}

inline const std::map<int, int>::const_iterator CharacterData::getKillCountBegin() const
{
    return mCharacterComponent->getKillCountBegin();
}

inline const std::map<int, int>::const_iterator CharacterData::getKillCountEnd() const
{
    return mCharacterComponent->getKillCountEnd();
}

inline void CharacterData::setKillCount(int monsterId, int kills)
{
    mCharacterComponent->setKillCount(monsterId, kills);
}

inline void CharacterData::clearAbilities()
{
    mEntity->getComponent<AbilityComponent>()->clearAbilities();
}

inline void CharacterData::giveAbility(int id, int mana)
{
    mEntity->getComponent<AbilityComponent>()->giveAbility(id, mana);
}

inline int CharacterData::getAbilitySize() const
{
    return mEntity->getComponent<AbilityComponent>()->getAbilities().size();
}

inline AbilityMap::const_iterator CharacterData::getAbilityBegin() const
{
    return mEntity->getComponent<AbilityComponent>()->getAbilities().begin();
}

inline AbilityMap::const_iterator CharacterData::getAbilityEnd() const
{
    return mEntity->getComponent<AbilityComponent>()->getAbilities().end();
}

inline Possessions &CharacterData::getPossessions() const
{
    return mCharacterComponent->getPossessions();
}

#endif // CHARACTER_H
