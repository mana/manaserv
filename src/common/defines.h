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

// Precomputed square-root of 2.
#define SQRT2 1.4142135623730950488

// World tick time in miliseconds.
#define WORLD_TICK_MS 100

/**
 * Exit value codes are thrown back at servers exit to reflect their exit state.
 */
enum ExitValue
{
    EXIT_NORMAL = 0,
    EXIT_CONFIG_NOT_FOUND, // The main configuration file wasn't found.
    EXIT_BAD_CONFIG_PARAMETER, // The configuration file has a wrong parameter.
    EXIT_XML_NOT_FOUND, // A required base xml configuration file wasn't found.
    EXIT_XML_BAD_PARAMETER, // The configuration of an xml file is faulty.
    EXIT_MAP_FILE_NOT_FOUND, // No map files found.
    EXIT_DB_EXCEPTION, // The database is invalid or unreachable.
    EXIT_NET_EXCEPTION, // The server was unable to start network connections.
    EXIT_OTHER_EXCEPTION
};

/**
 * Enumeration type for account levels.
 * A normal player would have permissions of 1
 * A tester would have permissions of 3 (AL_PLAYER | AL_TESTER)
 * A dev would have permissions of 7 (AL_PLAYER | AL_TESTER | AL_DEV)
 * A gm would have permissions of 11 (AL_PLAYER | AL_TESTER | AL_GM)
 * A admin would have permissions of 255 (*)
 */
enum AccessLevel
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
enum GuildAccessLevel
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
 * Damage type, used to know how to compute them.
 */
enum DamageType
{
    DAMAGE_PHYSICAL = 0,
    DAMAGE_MAGICAL,
    DAMAGE_DIRECT,
    DAMAGE_OTHER = -1
};

/**
 * A series of hardcoded attributes that must be defined.
 * FIXME: Much of these serve only to indicate derivatives, and so would not be
 * needed once this is no longer a hardcoded system.
 */
enum
{
    // Base Statistics
    ATTR_STR               = 1,
    ATTR_AGI               = 2,
    ATTR_VIT               = 3,
    ATTR_INT               = 4,
    ATTR_DEX               = 5,
    ATTR_WIL               = 6,

    // Derived attributes
    ATTR_ACCURACY          = 7,
    ATTR_DEFENSE           = 8,
    ATTR_DODGE             = 9,

    ATTR_MAGIC_DODGE       = 10,
    ATTR_MAGIC_DEFENSE     = 11,

    ATTR_BONUS_ASPD        = 12,

    ATTR_HP                = 13,
    ATTR_MAX_HP            = 14,
    ATTR_HP_REGEN          = 15,


    // Separate primary movespeed (tiles * second ^-1) and derived movespeed (raw)
    ATTR_MOVE_SPEED_TPS    = 16,
    ATTR_MOVE_SPEED_RAW    = 17,

    // Money and inventory size attributes.
    ATTR_GP                = 18,
    ATTR_INV_CAPACITY      = 19,

    /**
     * Temporary attributes.
     * @todo Use AutoAttacks instead.
     */
    MOB_ATTR_PHY_ATK_MIN   = 20,
    MOB_ATTR_PHY_ATK_DELTA = 21,
    MOB_ATTR_MAG_ATK       = 22
};

/**
 * Stackable types.
 * @todo non-stackable malus layers
 */
enum StackableType
{
    Stackable,
    NonStackable,
    NonStackableBonus
};

/**
 * Attribute augmentation methods.
 */
enum ModifierEffectType
{
    Multiplicative,
    Additive
};

struct AttributeInfoType
{
    StackableType stackableType;
    ModifierEffectType effectType;

    AttributeInfoType(StackableType s,
                      ModifierEffectType effect) :
        stackableType(s),
        effectType(effect)
    {}
};

#endif // DEFINES_H
