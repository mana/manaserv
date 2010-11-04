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

#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include <map>
#include <string>
#include <vector>

#include "common/inventorydata.hpp"
#include "game-server/being.hpp"
#include "protocol.h"
#include "defines.h"
#include "utils/logger.h"

class BuySell;
class GameClient;
class MessageIn;
class MessageOut;
class Point;
class Trade;

struct Special
{
    Special()
        : currentMana(0)
        , neededMana(0)
    {}

    int currentMana;
    int neededMana;
};

/**
 * The representation of a player's character in the game world.
 */
class Character : public Being
{
    public:
        /**
         * Utility constructor for creating a Character from a received
         * characterdata message.
         */
        Character(MessageIn &msg);

        ~Character();

        /**
         * recalculates the level when necessary and calls Being::update
         */
        void update();

        /**
         * Perform actions.
         */
        void perform();

        /**
         * Executes the global die script and calls the base class function
         */
        virtual void died();

        /**
         * makes the character respawn
         */
        void respawn();

        /**
         * makes the character perform a special action
         * when it is allowed to do so
         */
        void useSpecial(int id);

        /**
         * Allows a character to perform a special action
         */
        void giveSpecial(int id);

        /**
         * Removes all specials from character
         */
        void clearSpecials();

        /**
         * Checks if a character knows a special action
         */
        bool hasSpecial(int id) { return mSpecials.find(id) != mSpecials.end(); }

        /**
         * Removes an available special action
         */
        void takeSpecial(int id);

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

        /** Gets the gender of the character (male or female). */
        int getGender() const
        { return mGender; }

        /** Sets the gender of the character (male or female). */
        void setGender(int gender)
        { mGender = gender; }

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
        void sendStatus();

        /**
         * Gets the ID of the map that the character is on.
         * For serialization purpose only.
         */
        int getMapId() const;

        /**
         * Sets the ID of the map that the character is on.
         * For serialization purpose only.
         */
        void setMapId(int);

        /**
         * Marks all attributes as being modified.
         */
        void modifiedAllAttribute();

        /**
         * Recalculate the base value of an attribute and update derived
         *     attributes if it has changed.
         * @returns Whether it was changed.
         */
        bool recalculateBaseAttribute(unsigned int);

        /**
         * Attribute has changed, recalculate base value of dependant
         *     attributes (and handle other actions for the modified
         *     attribute)
         */
        void updateDerivedAttributes(unsigned int);

        /**
         * Calls all the "disconnected" listener.
         */
        void disconnected();

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
         * Used to serialize status effects.
         */
        int getStatusEffectSize() const
        { return mStatusEffects.size(); }

        const std::map<int, int>::const_iterator getStatusEffectBegin() const
        { return mStatusEffects.begin(); }

        const std::map<int, int>::const_iterator getStatusEffectEnd() const
        { return mStatusEffects.end(); }

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
         * Used to serialize specials.
         */
        int getSpecialSize() const
        { return mSpecials.size(); }

        const std::map<int, Special*>::const_iterator getSpecialBegin() const
        { return mSpecials.begin(); }

        const std::map<int, Special*>::const_iterator getSpecialEnd() const
        { return mSpecials.end(); }

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
        AttribmodResponseCode useCharacterPoint(size_t attribute);

        /**
         * Tries to use a correction point to reduce a
         * basic attribute and regain a character point
         */
        AttribmodResponseCode useCorrectionPoint(size_t attribute);

        void setCharacterPoints(int points) { mCharacterPoints = points; }
        int getCharacterPoints() const { return mCharacterPoints; }

        void setCorrectionPoints(int points) { mCorrectionPoints = points; }
        int getCorrectionPoints() const { return mCorrectionPoints; }

        /**
         * Gets the way the actor is blocked by other things on the map
         */
        virtual unsigned char getWalkMask() const
        { return 0x82; } // blocked by walls and monsters ( bin 1000 0010)

        /** Makes it impossible to chat for a while */
        void mute(int seconds)
        { setTimerHard(T_C_MUTE, seconds * 10); }

        bool isMuted() const
        { return isTimerRunning(T_C_MUTE); }

    protected:
        /**
         * Gets the way the actor blocks pathfinding for other objects
         */
        virtual Map::BlockType getBlockType() const
        { return Map::BLOCKTYPE_CHARACTER; }

    private:
        double getAttrBase(AttributeMap::const_iterator it) const
        { return it->second.getBase(); }
        double getAttrMod(AttributeMap::const_iterator it) const
        { return it->second.getModifiedAttribute(); }

        Character(const Character &);
        Character &operator=(const Character &);

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
        void levelup();

        /**
         * Marks attribute as recently modified.
         */
        void flagAttribute(int);

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
        void recalculateLevel();

        /**
         * Informs the client about his characters special charge status
         */
        void sendSpecialUpdate();

        enum TransactionType
        { TRANS_NONE, TRANS_TRADE, TRANS_BUYSELL };

        GameClient *mClient;   /**< Client computer. */
        /** Handler of the transaction the character is involved in. */
        void *mTransactionHandler;

        Possessions mPossessions;    /**< Possesssions of the character. */

        /** Attributes modified since last update. */
        std::set<size_t> mModifiedAttributes;
        std::set<size_t> mModifiedExperience;

        std::map<int, int> mExperience; /**< experience collected for each skill.*/

        std::map<int, Special*> mSpecials;
        std::map<int, int> mStatusEffects; /**< only used by select functions
                                                to make it easier to make the accountserver
                                                do not modify or use anywhere else*/
        int mRechargePerSpecial;
        bool mSpecialUpdateNeeded;

        int mDatabaseID;             /**< Character's database ID. */
        unsigned char mGender;       /**< Gender of the character. */
        unsigned char mHairStyle;    /**< Hair Style of the character. */
        unsigned char mHairColor;    /**< Hair Color of the character. */
        int mLevel;                  /**< Level of the character. */
        int mLevelProgress;          /**< progress to next level in percent */
        int mCharacterPoints;        /**< Unused attribute points that can be distributed */
        int mCorrectionPoints;       /**< Unused attribute correction points */
        bool mUpdateLevelProgress;   /**< Flag raised when percent to next level changed */
        bool mRecalculateLevel;      /**< Flag raised when the character level might have increased */
        unsigned char mAccountLevel; /**< Account level of the user. */
        int mParty;                  /**< Party id of the character */
        TransactionType mTransaction; /**< Trade/buy/sell action the character is involved in. */
        std::map<int, int> mKillCount;  /**< How many monsters the character has slain of each type */

        // Set as a friend, but still a lot of redundant accessors. FIXME.
        template< class T >
        friend void serializeCharacterData(const T &data, MessageOut &msg);
};

#endif // CHARACTER_HPP
