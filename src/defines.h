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


#ifndef _TMWSERV_DEFINES_H_
#define _TMWSERV_DEFINES_H_


/**
 * Enumeration type for account levels.
 *
 * Note the the actual tasks that can be done by admin or gm, or the
 * restrictions on a restricted user, are not specified yet. Also, banned
 * status will probably be derived from a date field (the time until an account
 * is banned).
 *
 * It may be better to wait and see what permissions we'd want to grant to or
 * take from users, and then come up with a convenient way to handle that.
 */
typedef enum {
    AL_NORMAL,      // User has regular rights
    AL_ADMIN,       // User can perform administrator tasks
    AL_GM,          // User can perform a subset of administrator tasks
    AL_BANNED,      // This user is currently banned
    AL_RESTRICTED   // User rights have been restricted
} AccountLevels;


/**
 * Enumeration type for the player genders.
 */
typedef enum {
    GENDER_MALE,
    GENDER_FEMALE,
    GENDER_UNKNOWN
} Genders;

/**
 * Enumerated type for communicated messages
 */
enum {
    // Login/Register
    CMSG_REGISTER                = 0x0000,
    CMSG_ENCRYPTED_REGISTER      = 0x0001,
    SMSG_REGISTER_RESPONSE       = 0x0002,
    CMSG_LOGIN                   = 0x0010,
    CMSG_ENCRYPTED_LOGIN         = 0x0011,
    SMSG_LOGIN_ERROR             = 0x0012,
    SMSG_LOGIN_CONFIRM           = 0x0013,
    CMSG_CHAR_CREATE             = 0x0020,
    SMSG_CHAR_CREATE_RESPONSE    = 0x0021,
    CMSG_CHAR_LIST               = 0x0022, // this is required after char creation
    CMSG_CHAR_SELECT             = 0x0023,
    SMSG_CHAR_SELECT_RESPONSE    = 0x0034,

    // Objects
    SMSG_NEW_OBJECT              = 0x0100,
    SMSG_REMOVE_OBJECT           = 0x0101,
    SMSG_CHANGE_OBJECT           = 0x0102,
    CMSG_PICKUP                  = 0x0110,
    SMSG_PICKUP_RESPONSE         = 0x0111,
    CMSG_USE_OBJECT              = 0x0120,
    SMSG_USE_RESPONSE            = 0x0121,

    // Beings
    SMSG_NEW_BEING               = 0x0200,
    SMSG_REMOVE_BEING            = 0x0201,
    SMSG_INVENTORY_UPD           = 0x0210,
    SMSG_EQUIPMENT_UPD           = 0x0220,
    SMSG_ATTACK                  = 0x0230,
    SMSG_PATH                    = 0x0240,
    CMSG_TARGET                  = 0x0250,
    CMSG_WALK                    = 0x0260,
    CMSG_START_TRADE             = 0x0270,
    CMSG_START_TALK              = 0x0280,
    CMSG_REQ_TRADE               = 0x0290,

    // Items
    CMSG_USE_ITEM                = 0x0300,
    CMSG_EQUIP                   = 0x0301,

    // Chat
    SMSG_CHAT                    = 0x0400,
    SMSG_SYSTEM                  = 0x0401,
    SMSG_ANNOUNCEMENT            = 0x0402,
    CMSG_SAY                     = 0x0410,
    CMSG_ANNOUNCE                = 0x0411,

    // Other
    SMSG_LOAD_MAP              = 0x0500,

    // NOTE: We will need more messages for in-game control (eg. moving a client to a new map/position etc.). Currently the protocol only caters for the bare basics.
};

// Login return values
enum {
    LOGIN_OK = 0,
    LOGIN_INVALID_USERNAME,
    LOGIN_INVALID_PASSWORD,
    LOGIN_INVALID_VERSION,
    LOGIN_SERVER_FULL,
    LOGIN_ACCOUNT_BANNED,
    LOGIN_ACCOUNT_REVIEW,
    LOGIN_UNKNOWN
};

// Account register return values
enum {
    REGISTER_OK = 0,
    REGISTER_INVALID_USERNAME,
    REGISTER_INVALID_PASSWORD,
    REGISTER_INVALID_EMAIL,
    REGISTER_EXISTS_USERNAME,
    REGISTER_EXISTS_EMAIL
};

// Character creation return values
enum {
    CREATE_OK = 0,
    CREATE_INVALID_NAME,
    CREATE_INVALID_HAIR,
    CREATE_INVALID_SEX,
    CREATE_EXISTS_USERNAME,
    CREATE_EXISTS_EMAIL,
    CREATE_NOLOGIN
};

// Character selection return values
enum {
    SELECT_OK = 0,
    SELECT_INVALID
};

// Object type enumeration
enum {
    OBJECT_ITEM = 0,
    OBJECT_PLAYER,
    OBJECT_MONSTER
};

// Pickup response enumeration
enum {
    PICKUP_OK = 0,
    PICKUP_OVERWEIGHT,
    PICKUP_FAIL
};

// Object use response enumeration
enum {
    USE_OK = 0,
    USE_FAIL
};


#endif // _TMWSERV_DEFINES_H_
