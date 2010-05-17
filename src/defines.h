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

#define SQRT2 1.4142135623730950488

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
     * Members with INVITE can invite other users
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
 * A series of hardcoded attributes that must be defined.
 * Much of these serve only to indicate derivatives, and so would not be
 * needed once this is no longer a hardcoded system.
 */

#define ATTR_STR                1
#define ATTR_AGI                2
#define ATTR_VIT                3
#define ATTR_INT                4
#define ATTR_DEX                5
#define ATTR_WIL                6

#define ATTR_ACCURACY           7
#define ATTR_DEFENSE            8
#define ATTR_DODGE              9

#define ATTR_MAGIC_DODGE        10
#define ATTR_MAGIC_DEFENSE      11

#define ATTR_BONUS_ASPD         12

#define ATTR_HP                 13
#define ATTR_MAX_HP             14
#define ATTR_HP_REGEN           15


// Separate primary movespeed (tiles * second ^-1) and derived movespeed (raw)
#define ATTR_MOVE_SPEED_TPS     16
#define ATTR_MOVE_SPEED_RAW     17
#define ATTR_GP                 18
#define ATTR_INV_CAPACITY       19

/**
 * Temporary attributes.
 * @todo Use AutoAttacks instead.
 */
#define MOB_ATTR_PHY_ATK_MIN    20
#define MOB_ATTR_PHY_ATK_DELTA  21
#define MOB_ATTR_MAG_ATK        22

/**
 * Attribute types. Can be one of stackable, non stackable, or non stackable bonus.
 * @todo non-stackable malus layers
 */

enum AT_TY {
    TY_ST,
    TY_NST,
    TY_NSTB,
    TY_NONE // Should only be used on types that have not yet been properly defined
};

enum AME_TY {
    AME_MULT,
    AME_ADD
};

struct AttributeInfoType {
        AT_TY sType;
        AME_TY eType;
        AttributeInfoType(AT_TY s, AME_TY e) : sType(s), eType(e) {}
};

#endif // DEFINES_H
