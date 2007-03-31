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
} AccountLevel;

enum
{
    // Network related
    MAX_CLIENTS  = 1024,

    // Chat related
/**
 * N.B: Private channels can't have an id less
 * than MAX_PUBLIC_CHANNELS_RANGE.
 */
    MAX_PUBLIC_CHANNELS_RANGE  = 1000,
    MAX_PRIVATE_CHANNELS_RANGE = 10000,
    MAX_CHANNEL_NAME           = 15,
    MAX_CHANNEL_ANNOUNCEMENT   = 150,
    MAX_CHANNEL_PASSWORD       = 12,

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
    MAX_HAIRSTYLE_VALUE = 7,
    MAX_HAIRCOLOR_VALUE = 9,
    MAX_GENDER_VALUE = 2,

/**
 * Points to give to a brand new character
 */
    POINTS_TO_DISTRIBUTES_AT_LVL1 = 70,

    // Screen Related
/**
 * Determine the area in which a character is aware of other beings
 */
    AROUND_AREA = 320
};

/**
 * Enumerated type for communicated messages
 * - PAMSG_*: from client to account server
 * - APMSG_*: from account server to client
 * - PCMSG_*: from client to chat server
 * - CPMSG_*: from chat server to client
 * - PGMSG_*: from client to game server
 * - GPMSG_*: from game server to client
 * Components: B byte, W word, D double word, S variable-size string
 *             C tile-based coordinates (B*3)
 */
enum {
    // Login/Register
    PAMSG_REGISTER                 = 0x0000, // L version, S username, S password, S email
    APMSG_REGISTER_RESPONSE        = 0x0002, // B error
    PAMSG_UNREGISTER               = 0x0003, // S username, S password
    APMSG_UNREGISTER_RESPONSE      = 0x0004, // B error
    PAMSG_LOGIN                    = 0x0010, // L version, S username, S password
    APMSG_LOGIN_RESPONSE           = 0x0012, // B error
    PAMSG_LOGOUT                   = 0x0013, // -
    APMSG_LOGOUT_RESPONSE          = 0x0014, // B error
    PAMSG_CHAR_CREATE              = 0x0020, // S name, B hair style, B hair color, B gender, W*6 stats
    APMSG_CHAR_CREATE_RESPONSE     = 0x0021, // B error
    PAMSG_CHAR_DELETE              = 0x0022, // B index
    APMSG_CHAR_DELETE_RESPONSE     = 0x0023, // B error
    APMSG_CHAR_INFO                = 0x0024, // B index, S name, B gender, B hair style, B hair color, B level, W money, W*6 stats
    PAMSG_CHAR_SELECT              = 0x0026, // B index
    APMSG_CHAR_SELECT_RESPONSE     = 0x0027, // B error, B*32 token, S game address, W game port, S chat address, W chat port
    PAMSG_EMAIL_CHANGE             = 0x0030, // S email
    APMSG_EMAIL_CHANGE_RESPONSE    = 0x0031, // B error
    PAMSG_EMAIL_GET                = 0x0032, // -
    APMSG_EMAIL_GET_RESPONSE       = 0x0033, // B error, S email
    PAMSG_PASSWORD_CHANGE          = 0x0034, // S old password, S new password
    APMSG_PASSWORD_CHANGE_RESPONSE = 0x0035, // B error

    PGMSG_CONNECT                  = 0x0050, // B*32 token
    GPMSG_CONNECT_RESPONSE         = 0x0051, // B error
    PCMSG_CONNECT                  = 0x0053, // B*32 token
    CPMSG_CONNECT_RESPONSE         = 0x0054, // B error

    PGMSG_DISCONNECT               = 0x0060, // B reconnect account
    GPMSG_DISCONNECT_RESPONSE      = 0x0061, // B error, B*32 token
    PCMSG_DISCONNECT               = 0x0063, // -
    CPMSG_DISCONNECT_RESPONSE      = 0x0064, // B error

    PAMSG_RECONNECT                = 0x0065, // B*32 token
    APMSG_RECONNECT_RESPONSE       = 0x0066, // B error

    APMSG_CONNECTION_TIMEDOUT      = 0x0070, // -
    GPMSG_CONNECTION_TIMEDOUT      = 0x0071, // -

    // Game
    GPMSG_PLAYER_MAP_CHANGE        = 0x0100, // S filename, W x, W y
    GPMSG_PLAYER_SERVER_CHANGE     = 0x0101, // B*32 token, S game address, W game port
    PGMSG_PICKUP                   = 0x0110, // W*2 position
    PGMSG_DROP                     = 0x0111, // B slot, B amount
    PGMSG_EQUIP                    = 0x0112, // B slot
    GPMSG_INVENTORY                = 0x0120, // { B slot, W item id [, B amount] }*
    GPMSG_INVENTORY_FULL           = 0x0121, // { B slot, W item id [, B amount] }*
    GPMSG_PLAYER_ATTRIBUTE_UPDATE  = 0x0130, // { W attribute, W value }*
    GPMSG_BEING_ENTER              = 0x0200, // B type, W being id, B action, W*2 position
                                             // character: S name, B hair style, B hair color, B gender
                                             // monster: W type id
    GPMSG_BEING_LEAVE              = 0x0201, // W being id
    GPMSG_ITEM_APPEAR              = 0x0202, // W item id, W*2 position
    PGMSG_WALK                     = 0x0260, // W*2 destination
    PGMSG_ACTION_CHANGE            = 0x0270, // B Action
    GPMSG_BEING_ACTION_CHANGE      = 0x0271, // W being id, B action
    GPMSG_BEINGS_MOVE              = 0x0280, // { W being id, B flags [, C position] [, W*2 destination] }*
    GPMSG_ITEMS                    = 0x0281, // { W item id, W*2 position }*
    PGMSG_ATTACK                   = 0x0290, // B direction
    GPMSG_BEING_ATTACK             = 0x0291, // W being id
    PGMSG_SAY                      = 0x02A0, // S text
    GPMSG_SAY                      = 0x02A1, // W being id, S text
    PGMSG_USE_ITEM                 = 0x0300, // L item id
    GPMSG_USE_RESPONSE             = 0x0301, // B error
    GPMSG_BEINGS_DAMAGE            = 0x0310, // { W being id, W amount }*

    // Chat
    CPMSG_ERROR                    = 0x0401, // B error
    CPMSG_ANNOUNCEMENT             = 0x0402, // S text
    CPMSG_PRIVMSG                  = 0x0403, // S user, S text
    CPMSG_PUBMSG                   = 0x0404, // W channel, S user, S text
    PCMSG_CHAT                     = 0x0410, // S text, W channel
    PCMSG_ANNOUNCE                 = 0x0411, // S text
    PCMSG_PRIVMSG                  = 0x0412, // S user, S text
    // -- Channeling
    PCMSG_REGISTER_CHANNEL            = 0x0413, // B pub/priv, S name, S announcement, S password
    CPMSG_REGISTER_CHANNEL_RESPONSE   = 0x0414, // B error, W id
    PCMSG_UNREGISTER_CHANNEL          = 0x0415, // W channel
    CPMSG_UNREGISTER_CHANNEL_RESPONSE = 0x0416, // B error
    CPMSG_CHANNEL_EVENT               = 0x0418, // W channel, B event, S user
    PCMSG_ENTER_CHANNEL               = 0x0419, // S channel, S password
    CPMSG_ENTER_CHANNEL_RESPONSE      = 0x0420, // B error, W channel
    PCMSG_QUIT_CHANNEL                = 0x0421, // W channel
    CPMSG_QUIT_CHANNEL_RESPONSE       = 0x0422, // B error
    PCMSG_LIST_CHANNELS               = 0x0423, // -
    CPMSG_LIST_CHANNELS_RESPONSE      = 0x0424, // W number of channels, S channels

    // Inter-server
    GAMSG_REGISTER     = 0x0500, // S address, W port, { W map id }*
    AGMSG_ACTIVE_MAP   = 0x0501, // W map id
    AGMSG_PLAYER_ENTER = 0x0510, // B*32 token, serialised character data
    GAMSG_PLAYER_DATA  = 0x0520, // serialised character data
    GAMSG_REDIRECT          = 0x0530, // L id
    AGMSG_REDIRECT_RESPONSE = 0x0531, // L id, B*32 token, S game address, W game port
    GAMSG_PLAYER_RECONNECT  = 0x0532, // L id, B*32 token

    XXMSG_INVALID = 0x7FFF
};

// Generic return values

enum {
    ERRMSG_OK = 0,                      // everything is fine
    ERRMSG_FAILURE,                     // the action failed
    ERRMSG_NO_LOGIN,                    // the user is not yet logged
    ERRMSG_NO_CHARACTER_SELECTED,       // the user needs a character
    ERRMSG_INSUFFICIENT_RIGHTS,         // the user is not privileged
    ERRMSG_INVALID_ARGUMENT             // part of the received message was invalid
};

// Login specific return values
enum {
    LOGIN_INVALID_VERSION = 0x40,       // the user is using an incompatible protocol
    LOGIN_SERVER_FULL                   // the server is overloaded
};

// Account register specific return values
enum {
    REGISTER_INVALID_VERSION = 0x40,    // the user is using an incompatible protocol
    REGISTER_EXISTS_USERNAME,           // there already is an account with this username
    REGISTER_EXISTS_EMAIL               // there already is an account with this email address
};

// Character creation specific return values
enum {
    CREATE_INVALID_HAIRSTYLE = 0x40,
    CREATE_INVALID_HAIRCOLOR,
    CREATE_INVALID_GENDER,
    CREATE_ATTRIBUTES_TOO_HIGH,
    CREATE_ATTRIBUTES_TOO_LOW,
    CREATE_ATTRIBUTES_EQUAL_TO_ZERO,
    CREATE_EXISTS_NAME,
    CREATE_TOO_MUCH_CHARACTERS
};

// Email change specific return values
enum {
    EMAILCHG_EXISTS_EMAIL = 0x40
};

// Chat errors return values
enum {
    CHAT_USING_BAD_WORDS = 0x40,
    CHAT_UNHANDLED_COMMAND
};

// Chat channels event values
enum {
    CHAT_EVENT_NEW_PLAYER = 0,
    CHAT_EVENT_LEAVING_PLAYER
};

// Moving object flags
enum {
    // Payload contains the current position.
    MOVING_POSITION = 1,
    // Payload contains the destination.
    MOVING_DESTINATION = 2
};

/**
 * Enumerations for the handling of attributes and their modifiers.
 */

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
 * Stats every being needs
 */
enum BeingStats
{
    BASE_ATTR_STRENGTH = 0,    // Basic attributes
    BASE_ATTR_AGILITY,
    BASE_ATTR_DEXTERITY,
    BASE_ATTR_VITALITY,
    BASE_ATTR_INTELLIGENCE,
    BASE_ATTR_WILLPOWER,
    BASE_ATTR_CHARISMA,
    NB_BASE_ATTRIBUTES,

    ATTR_EFF_STRENGTH = NB_BASE_ATTRIBUTES,    // modified basic attributes
    ATTR_EFF_AGILITY,
    ATTR_EFF_DEXTERITY,
    ATTR_EFF_VITALITY,
    ATTR_EFF_INTELLIGENCE,
    ATTR_EFF_WILLPOWER,
    ATTR_EFF_CHARISMA,
    NB_EFFECTIVE_ATTRIBUTES,

    DERIVED_ATTR_HP_MAXIMUM = NB_EFFECTIVE_ATTRIBUTES,    // Computed stats
    DERIVED_ATTR_PHYSICAL_ATTACK_MINIMUM,
    DERIVED_ATTR_PHYSICAL_ATTACK_FLUCTUATION,
    DERIVED_ATTR_PHYSICAL_DEFENCE,
    // add new computed statistics when they are needed
    NB_ATTRIBUTES_BEING
};


/**
 * Player character specific stats
 */
enum CharacterStats
{
    CHAR_SKILL_WEAPON_UNARMED = NB_ATTRIBUTES_BEING,
    CHAR_SKILL_WEAPON_SWORD,
    CHAR_SKILL_WEAPON_AXE,
    CHAR_SKILL_WEAPON_POLEARM,
    CHAR_SKILL_WEAPON_JAVELIN,
    CHAR_SKILL_WEAPON_WHIP,
    CHAR_SKILL_WEAPON_DAGGER,
    CHAR_SKILL_WEAPON_STAFF,
    CHAR_SKILL_WEAPON_BOW,
    CHAR_SKILL_WEAPON_CROSSBOW,
    CHAR_SKILL_WEAPON_THROWN,
    NB_CHAR_WEAPONSKILLS,

    CHAR_SKILL_MAGIC_IAMJUSTAPLACEHOLDER = NB_CHAR_WEAPONSKILLS,
    NB_CHAR_MAGICSKILLS,

    CHAR_SKILL_CRAFT_IAMJUSTAPLACEHOLDER = NB_CHAR_MAGICSKILLS,
    NB_CHAR_CRAFTSKILLS,

    CHAR_SKILL_IAMJUSTAPLACEHOLDER = NB_CHAR_CRAFTSKILLS,
    NB_CHAR_OTHERSKILLS,

    NB_ATTRIBUTES_CHAR = NB_CHAR_OTHERSKILLS
};


/**
 * Monster-specific stats
 */
enum MonsterStats
{
    MONSTER_SKILL_WEAPON = NB_ATTRIBUTES_BEING,
    NB_ATTRIBUTES_CONTROLLED
};


#endif // _TMWSERV_DEFINES_H_
