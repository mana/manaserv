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

#ifndef _TMWSERV_PLAYER_H_
#define _TMWSERV_PLAYER_H_

#include <string>
#include <vector>

#include "playerdata.hpp"
#include "game-server/being.hpp"

class GameClient;

/**
 * Actions for a player being.
 */
enum
{
    PLAYER_STAND = 0,
    PLAYER_SIT,
    PLAYER_ATTACK
};

/**
 * Stores the data of a remote client.
 */
class Player : public Being, public PlayerData
{
    public:

        /**
         * Base attributes of a player character
         */
        enum Attributes
        {
            STRENGTH = NB_STATS_BEING,
            AGILITY,
            DEXTERITY,
            VITALITY,
            INTELLIGENCE,
            WILLPOWER,
            CHARISMA,
            NB_ATTRIBUTES
        };

        enum WeaponSkills
        {
            SKILL_WEAPON_UNARMED = NB_ATTRIBUTES,
            SKILL_WEAPON_SWORD,
            SKILL_WEAPON_AXE,
            SKILL_WEAPON_POLEARM,
            SKILL_WEAPON_JAVELIN,
            SKILL_WEAPON_WHIP,
            SKILL_WEAPON_DAGGER,
            SKILL_WEAPON_STAFF,
            SKILL_WEAPON_BOW,
            SKILL_WEAPON_CROSSBOW,
            SKILL_WEAPON_THROWN,
            NB_WEAPONSKILLS
        };

        enum MagicSkills
        {
            SKILL_MAGIC_IAMJUSTAPLACEHOLDER = NB_WEAPONSKILLS,
            NB_MAGICSKILLS
        };

        enum CraftSkills
        {
            SKILL_CRAFT_IAMJUSTAPLACEHOLDER = NB_MAGICSKILLS,
            NB_CRAFTSKILLS
        };

        enum OtherSkills
        {
            SKILL_IAMJUSTAPLACEHOLDER = NB_CRAFTSKILLS,
            NB_OTHERSKILLS
        }

        static const NB_STATS_PLAYER = NB_OTHERSKILLS;

        Player(std::string const &name, int id = -1);

        /**
         * Updates the internal status.
         */
        void update();

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
         * Recalculates all player stats that are derived from others.
         * Call whenever you change something that affects a derived stat.
         * Called automatically when you manipulate a stat using setBaseStat()
         */
        virtual void calculateBaseStats();


    private:
        Player(Player const &);
        Player &operator=(Player const &);

        GameClient *mClient;   /**< Client computer. */
};

#endif // _TMWSERV_PLAYER_H_
