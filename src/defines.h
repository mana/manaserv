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

#ifndef DEFINES_H
#define DEFINES_H

/**
 * Enumeration type for account levels.
 * A normal player would have permissions of 1
 * A tester would have permissions of 3 (AL_PLAYER | AL_TESTER)
 * A dev would have permissions of 7 (AL_PLAYER | AL_TESTER | AL_DEV)
 * A gm would have permissions of 11 (AL_PLAYER | AL_TESTER | AL_GM)
 * A admin would have permissions of 255 (*)
 */
enum
{
    AL_BANNED =   0,     /**< This user is currently banned. */
    AL_PLAYER =   1,     /**< User has regular rights. */
    AL_TESTER =   2,     /**< User can perform testing tasks. */
    AL_DEV    =   4,     /**< User is a developer and can perform dev tasks */
    AL_GM     =   8,     /**< User is a moderator and can perform mod tasks */
    AL_ADMIN  =  128     /**< User can perform administrator tasks. */
};

/**
     * Guild member permissions
     * Members with NONE cannot invite users or set permissions
     * Members with TOPIC_CHANGE can change the guild channel topic
     * Members with INVIT can invite other users
     * Memeber with KICK can remove other users
     * Members with OWNER can invite users and set permissions
     */
enum
{
    GAL_NONE = 0,
    GAL_TOPIC_CHANGE = 1,
    GAL_INVITE = 2,
    GAL_KICK = 4,
    GAL_OWNER = 255
};

/**
 * Determine the default area in which a character is aware of other beings
 */
const int DEFAULT_INTERACTION_TILES_AREA = 20;
/**
 * Default tile length in pixel
 */
const int DEFAULT_TILE_LENGTH = 32;

/**
 * Possible states of beings.
 * States can be multiple for the same being.
 * To be used as bitmask values.
 */
enum BeingState
{
    STATE_POISONED = 1,
    STATE_STONED   = 2,
    STATE_STUNNED  = 4,
    STATE_SLOWED   = 8,
    STATE_TIRED    = 16,
    STATE_MAD      = 32,
    STATE_BERSERK  = 64,
    STATE_HASTED   = 128,
    STATE_FLOATING = 256
};

/**
 * Element attribute for beings, actors, and items.
 * Subject to change until Pauan and Dabe are finished with the element system.
 * Please keep element modifier of BeingAttribute in sync.
 */
enum Element
{
    ELEMENT_NEUTRAL = 0,
    ELEMENT_FIRE,
    ELEMENT_WATER,
    ELEMENT_EARTH,
    ELEMENT_AIR,
    ELEMENT_LIGHTNING,
    ELEMENT_METAL,
    ELEMENT_WOOD,
    ELEMENT_ICE,
    ELEMENT_ILLEGAL
};

/**
 * Attributes used during combat. Available to all the beings.
 */
enum
{
    BASE_ATTR_BEGIN = 0,
    BASE_ATTR_PHY_ATK_MIN = BASE_ATTR_BEGIN,
    BASE_ATTR_PHY_ATK_DELTA,
                       /**< Physical attack power. */
    BASE_ATTR_MAG_ATK, /**< Magical attack power. */
    BASE_ATTR_PHY_RES, /**< Resistance to physical damage. */
    BASE_ATTR_MAG_RES, /**< Resistance to magical damage. */
    BASE_ATTR_EVADE,   /**< Ability to avoid hits. */
    BASE_ATTR_HIT,     /**< Ability to hit stuff. */
    BASE_ATTR_HP,      /**< Hit Points (Base value: maximum, Modded value: current) */
    BASE_ATTR_HP_REGEN,/**< number of HP regenerated every 10 game ticks */
    BASE_ATTR_END,
    BASE_ATTR_NB = BASE_ATTR_END - BASE_ATTR_BEGIN,

    BASE_ELEM_BEGIN = BASE_ATTR_END,
    BASE_ELEM_NEUTRAL = BASE_ELEM_BEGIN,
    BASE_ELEM_FIRE,
    BASE_ELEM_WATER,
    BASE_ELEM_EARTH,
    BASE_ELEM_AIR,
    BASE_ELEM_SACRED,
    BASE_ELEM_DEATH,
    BASE_ELEM_END,
    BASE_ELEM_NB = BASE_ELEM_END - BASE_ELEM_BEGIN,

    NB_BEING_ATTRIBUTES = BASE_ELEM_END
};

/**
 * Attributes of characters. Used to derive being attributes.
 */
enum
{
    CHAR_ATTR_BEGIN = NB_BEING_ATTRIBUTES,
    CHAR_ATTR_STRENGTH = CHAR_ATTR_BEGIN,
    CHAR_ATTR_AGILITY,
    CHAR_ATTR_DEXTERITY,
    CHAR_ATTR_VITALITY,
    CHAR_ATTR_INTELLIGENCE,
    CHAR_ATTR_WILLPOWER,
    CHAR_ATTR_END,
    CHAR_ATTR_NB = CHAR_ATTR_END - CHAR_ATTR_BEGIN
};

#endif // DEFINES_H
