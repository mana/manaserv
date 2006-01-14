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

    // Network related
const unsigned int MAX_CLIENTS  = 1024,

    // Registering related
    MIN_LOGIN_LENGTH = 4,
    MAX_LOGIN_LENGTH = 16,
    MIN_PASSWORD_LENGTH = 4,
    MAX_PASSWORD_LENGTH = 25,
    MIN_EMAIL_LENGTH = 7,
    MAX_EMAIL_LENGTH = 50,

    // Character related
    MIN_CHARACTER_LENGTH = 4,
    MAX_CHARACTER_LENGTH = 25,
    MAX_OF_CHARACTERS = 3,
    MAX_HAIRSTYLE_VALUE = 5,
    MAX_HAIRCOLOR_VALUE = 10,
    MAX_GENDER_VALUE = 2,
/** Tells the max difference between the
 *  less big stat and the biggest one.
 *  So that players haven't disproportionned
 *  Raw statistics.
 */
    MAX_DIFF_BETWEEN_STATS = 5,
/**
 * Points to give to a brand new character
 */
    POINTS_TO_DISTRIBUTES_AT_LVL1 = 60,

    // Screen Related
/**
 * Determine the area in which a character
 * can hear another one speak
 */
    AROUND_AREA_IN_TILES = 10;


/**
 * Enumerated type for communicated messages
 */
enum {
    // Login/Register
    CMSG_REGISTER                 = 0x0000,
    CMSG_ENCRYPTED_REGISTER       = 0x0001,
    SMSG_REGISTER_RESPONSE        = 0x0002,
    CMSG_UNREGISTER               = 0x0003,
    SMSG_UNREGISTER_RESPONSE      = 0x0004,
    CMSG_LOGIN                    = 0x0010,
    CMSG_ENCRYPTED_LOGIN          = 0x0011,
    SMSG_LOGIN_RESPONSE           = 0x0012,
    CMSG_LOGOUT                   = 0x0013,
    SMSG_LOGOUT_RESPONSE          = 0x0014,
    CMSG_CHAR_CREATE              = 0x0020,
    SMSG_CHAR_CREATE_RESPONSE     = 0x0021,
    CMSG_CHAR_DELETE              = 0x0022,
    SMSG_CHAR_DELETE_RESPONSE     = 0x0023,
    CMSG_CHAR_LIST                = 0x0024, // this is required after char creation
    SMSG_CHAR_LIST_RESPONSE       = 0x0025,
    CMSG_CHAR_SELECT              = 0x0026,
    SMSG_CHAR_SELECT_RESPONSE     = 0x0027,
    CMSG_EMAIL_CHANGE             = 0x0030,
    SMSG_EMAIL_CHANGE_RESPONSE    = 0x0031,
    CMSG_EMAIL_GET                = 0x0032,
    SMSG_EMAIL_GET_RESPONSE       = 0x0033,
    CMSG_FORGOT_PASSWORD          = 0x0040,
    SMSG_FORGOT_PASSWORD_RESPONSE = 0x0041,
    CMSG_PASSWORD_CHANGE          = 0x0050,
    SMSG_PASSWORD_CHANGE_RESPONSE = 0x0051,

    // Objects
    SMSG_NEW_OBJECT               = 0x0100,
    SMSG_REMOVE_OBJECT            = 0x0101,
    SMSG_CHANGE_OBJECT            = 0x0102,
    CMSG_PICKUP                   = 0x0110,
    SMSG_PICKUP_RESPONSE          = 0x0111,
    CMSG_USE_OBJECT               = 0x0120,
    SMSG_USE_RESPONSE             = 0x0121,

    // Beings
    SMSG_NEW_BEING                = 0x0200,
    SMSG_REMOVE_BEING             = 0x0201,
    SMSG_INVENTORY_UPD            = 0x0210,
    SMSG_EQUIPMENT_UPD            = 0x0220,
    SMSG_ATTACK                   = 0x0230,
    SMSG_PATH                     = 0x0240,
    CMSG_TARGET                   = 0x0250,
    CMSG_WALK                     = 0x0260,
    CMSG_START_TRADE              = 0x0270,
    CMSG_START_TALK               = 0x0280,
    CMSG_REQ_TRADE                = 0x0290,

    // Items
    CMSG_USE_ITEM                 = 0x0300,
    CMSG_EQUIP                    = 0x0301,
    SMSG_EQUIP_RESPONSE           = 0x0302,

    // Chat
    SMSG_SYSTEM                   = 0x0400,
    SMSG_CHAT                     = 0x0401,
    SMSG_ANNOUNCEMENT             = 0x0402,
    SMSG_PRIVMSG                  = 0x0403,
    CMSG_SAY                      = 0x0410,
    CMSG_ANNOUNCE                 = 0x0411,
    CMSG_PRIVMSG                  = 0x0412,

    // Other
    SMSG_LOAD_MAP                 = 0x0500,

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
    LOGIN_ALREADY_LOGGED,
    LOGIN_UNKNOWN
};

// Logout return values
enum {
    LOGOUT_OK = 0,
    LOGOUT_UNSUCCESSFULL
};

// Account register return values
enum {
    REGISTER_OK = 0,
    REGISTER_INVALID_USERNAME,
    REGISTER_INVALID_PASSWORD,
    REGISTER_INVALID_EMAIL,
    REGISTER_INVALID_VERSION,
    REGISTER_EXISTS_USERNAME,
    REGISTER_EXISTS_EMAIL,
    REGISTER_UNKNOWN
};

// Account deletion return values
enum {
    UNREGISTER_OK = 0,
    UNREGISTER_INVALID_USERNAME,
    UNREGISTER_INVALID_PASSWORD,
    UNREGISTER_INVALID_UNSUFFICIENT_RIGHTS,
    UNREGISTER_UNKNOWN
};

// Character creation return values
enum {
    CREATE_OK = 0,
    CREATE_INVALID_NAME,
    CREATE_INVALID_HAIRSTYLE,
    CREATE_INVALID_HAIRCOLOR,
    CREATE_INVALID_GENDER,
    CREATE_RAW_STATS_TOO_HIGH,
    CREATE_RAW_STATS_TOO_LOW,
    CREATE_RAW_STATS_INVALID_DIFF,
    CREATE_RAW_STATS_EQUAL_TO_ZERO,
    CREATE_EXISTS_NAME,
    CREATE_TOO_MUCH_CHARACTERS,
    CREATE_NOLOGIN,
    CREATE_UNKNOWN
};

// Character deletion return values
enum {
    DELETE_OK = 0,
    DELETE_INVALID_NAME,
    DELETE_NO_MORE_CHARACTERS,
    DELETE_NOLOGIN,
    DELETE_UNKNOWN
};

// Character selection return values
// (When selecting a new one, you deselect the previous.)
enum {
    SELECT_OK = 0,
    SELECT_INVALID,
    SELECT_NO_CHARACTERS,
    SELECT_NOLOGIN,
    SELECT_UNKNOWN
};

// Character's list return values
enum {
    CHAR_LIST_OK = 0,
    CHAR_LIST_NOLOGIN,
    CHAR_LIST_UNKNOWN
};

// Email change return values
enum {
    EMAILCHG_OK = 0,
    EMAILCHG_NOLOGIN,
    EMAILCHG_INVALID,
    EMAILCHG_EXISTS_EMAIL,
    EMAILCHG_UNKNOWN
};

// Get Email return values
enum {
    EMAILGET_OK = 0,
    EMAILGET_NOLOGIN,
    EMAILGET_UNKNOWN
};

// Password change return values
enum {
    PASSCHG_OK = 0,
    PASSCHG_NOLOGIN,
    PASSCHG_INVALID,
    PASSCHG_MISMATCH,
    PASSCHG_UNKNOWN
};

// Chat errors return values
enum {
    // CHAT_OK = 0,
    CHAT_NOLOGIN = 1,
    CHAT_NO_CHARACTER_SELECTED,
    CHAT_USING_BAD_WORDS,
    CHATCMD_UNHANDLED_COMMAND,
    CHATCMD_UNSUFFICIENT_RIGHTS,
    CHATCMD_UNKNOWN
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

// Equip responses
enum {
    EQUIP_OK = 0,
    EQUIP_FAIL
};

#endif // _TMWSERV_DEFINES_H_
