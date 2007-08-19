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
enum AccountLevel
{
    AL_NORMAL,      // User has regular rights
    AL_ADMIN,       // User can perform administrator tasks
    AL_GM,          // User can perform a subset of administrator tasks
    AL_BANNED,      // This user is currently banned
    AL_RESTRICTED   // User rights have been restricted
};

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
    CPMSG_CONNECTION_TIMEDOUT      = 0x0072, // -

    // Game
    GPMSG_PLAYER_MAP_CHANGE        = 0x0100, // S filename, W x, W y
    GPMSG_PLAYER_SERVER_CHANGE     = 0x0101, // B*32 token, S game address, W game port
    PGMSG_PICKUP                   = 0x0110, // W*2 position
    PGMSG_DROP                     = 0x0111, // B slot, B amount
    PGMSG_EQUIP                    = 0x0112, // B slot
    PGMSG_UNEQUIP                  = 0x0113, // B slot
    PGMSG_MOVE_ITEM                = 0x0114, // B slot1, B slot2, B amount
    GPMSG_INVENTORY                = 0x0120, // { B slot, W item id [, B amount] }*
    GPMSG_INVENTORY_FULL           = 0x0121, // { B slot, W item id [, B amount] }*
    GPMSG_PLAYER_ATTRIBUTE_CHANGE  = 0x0130, // { B attribute, W base value, W modified value }*
    GPMSG_BEING_ENTER              = 0x0200, // B type, W being id, B action, W*2 position
                                             // character: S name, B hair style, B hair color, B gender, B item bitmask, { W item id }*
                                             // monster: W type id
                                             // npc: W type id
    GPMSG_BEING_LEAVE              = 0x0201, // W being id
    GPMSG_ITEM_APPEAR              = 0x0202, // W item id, W*2 position
    GPMSG_BEING_LOOKS_CHANGE       = 0x0210, // W weapon, W hat, W top clothes, W bottom clothes
    PGMSG_WALK                     = 0x0260, // W*2 destination
    PGMSG_ACTION_CHANGE            = 0x0270, // B Action
    GPMSG_BEING_ACTION_CHANGE      = 0x0271, // W being id, B action
    GPMSG_BEINGS_MOVE              = 0x0280, // { W being id, B flags [, C position, B speed] [, W*2 destination] }*
    GPMSG_ITEMS                    = 0x0281, // { W item id, W*2 position }*
    PGMSG_ATTACK                   = 0x0290, // B direction
    GPMSG_BEING_ATTACK             = 0x0291, // W being id, B direction
    PGMSG_SAY                      = 0x02A0, // S text
    GPMSG_SAY                      = 0x02A1, // W being id, S text
    GPMSG_NPC_CHOICE               = 0x02B0, // W being id, { S text }*
    GPMSG_NPC_MESSAGE              = 0x02B1, // W being id, B* text
    PGMSG_NPC_TALK                 = 0x02B2, // W being id
    PGMSG_NPC_TALK_NEXT            = 0x02B3, // W being id
    PGMSG_NPC_SELECT               = 0x02B4, // W being id, B choice
    GPMSG_NPC_BUY                  = 0x02B5, // W being id, { W item id, W amount, W cost }*
    GPMSG_NPC_SELL                 = 0x02B6, // W being id, { W item id, W amount, W cost }*
    PGMSG_NPC_BUYSELL              = 0x02B7, // W item id, W amount
    PGMSG_TRADE_REQUEST            = 0x02C0, // W being id
    GPMSG_TRADE_REQUEST            = 0x02C1, // W being id
    GPMSG_TRADE_START              = 0x02C2, // -
    GPMSG_TRADE_COMPLETE           = 0x02C3, // -
    PGMSG_TRADE_CANCEL             = 0x02C4, // -
    GPMSG_TRADE_CANCEL             = 0x02C5, // -
    PGMSG_TRADE_ACCEPT             = 0x02C6, // -
    GPMSG_TRADE_ACCEPT             = 0x02C7, // -
    PGMSG_TRADE_ADD_ITEM           = 0x02C8, // B slot, B amount
    GPMSG_TRADE_ADD_ITEM           = 0x02C9, // W item id, B amount
    PGMSG_TRADE_SET_MONEY          = 0x02CA, // L amount
    GPMSG_TRADE_SET_MONEY          = 0x02CB, // L amount
    PGMSG_USE_ITEM                 = 0x0300, // B slot
    GPMSG_USE_RESPONSE             = 0x0301, // B error
    GPMSG_BEINGS_DAMAGE            = 0x0310, // { W being id, W amount }*

#if 0
    // Guild
    PGMSG_GUILD_CREATE                  = 0x0350, // S name
    GPMSG_GUILD_CREATE_RESPONSE         = 0x0351, // B error, W id
    PGMSG_GUILD_INVITE                  = 0x0352, // W id, S name
    GPMSG_GUILD_INVITE_RESPONSE         = 0x0353, // B error
    PGMSG_GUILD_ACCEPT                  = 0x0354, // S name
    GPMSG_GUILD_ACCEPT_RESPONSE         = 0x0355, // B error
    PGMSG_GUILD_GET_MEMBERS             = 0x0356, // W id
    GPMSG_GUILD_GET_MEMBERS_RESPONSE    = 0x0357, // S names
    GPMSG_GUILD_JOINED                  = 0x0358, // W id, S name
    GPMSG_GUILD_LEFT                    = 0x0359, // W id
    PGMSG_GUILD_QUIT                    = 0x0360, // W id
    GPMSG_GUILD_QUIT_RESPONSE           = 0x0361, // B error, W id
#endif

    CPMSG_GUILD_INVITED                 = 0x0370, // S name, S name
    CPMSG_GUILD_REJOIN                  = 0x0371, // S name, W id, W rights

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
    CPMSG_USERJOINED                  = 0x0425, // W channel, S name
    CPMSG_USERLEFT                    = 0x0426, // W channel, S name
    PCMSG_LIST_CHANNELUSERS           = 0x0427, // S channel
    CPMSG_LIST_CHANNELUSERS_RESPONSE  = 0x0428, // S users

    // Inter-server
    GAMSG_REGISTER     = 0x0500, // S address, W port, { W map id }*
    AGMSG_ACTIVE_MAP   = 0x0501, // W map id
    AGMSG_PLAYER_ENTER = 0x0510, // B*32 token, L id, S name, serialised character data
    GAMSG_PLAYER_DATA  = 0x0520, // L id, serialised character data
    GAMSG_REDIRECT          = 0x0530, // L id
    AGMSG_REDIRECT_RESPONSE = 0x0531, // L id, B*32 token, S game address, W game port
    GAMSG_PLAYER_RECONNECT  = 0x0532, // L id, B*32 token

#if 0
    GAMSG_GUILD_CREATE                  = 0x0550, // S name
    AGMSG_GUILD_CREATE_RESPONSE         = 0x0551, // B error, W id
    GAMSG_GUILD_INVITE                  = 0x0552, // W id, S name
    AGMSG_GUILD_INVITE_RESPONSE         = 0x0553, // B error
    GAMSG_GUILD_ACCEPT                  = 0x0554, // S name
    AGMSG_GUILD_ACCEPT_RESPONSE         = 0x0555, // B error
    GAMSG_GUILD_GET_MEMBERS             = 0x0556, // W id
    AGMSG_GUILD_GET_MEMBERS_RESPONSE    = 0x0557, // S names
    GAMSG_GUILD_QUIT                    = 0x0558, // W id
    AGMSG_GUILD_QUIT_RESPONSE           = 0x0559, // B error
#endif

    XXMSG_INVALID = 0x7FFF
};

// Generic return values

enum {
    ERRMSG_OK = 0,                      // everything is fine
    ERRMSG_FAILURE,                     // the action failed
    ERRMSG_NO_LOGIN,                    // the user is not yet logged
    ERRMSG_NO_CHARACTER_SELECTED,       // the user needs a character
    ERRMSG_INSUFFICIENT_RIGHTS,         // the user is not privileged
    ERRMSG_INVALID_ARGUMENT,            // part of the received message was invalid
    ERRMSG_ALREADY_TAKEN                // name used was already taken
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
enum
{
    ELEMENT_NEUTRAL = 0,
    ELEMENT_FIRE,
    ELEMENT_WATER,
    ELEMENT_EARTH,
    ELEMENT_AIR,
    ELEMENT_SACRED,
    ELEMENT_DEATH
};

/**
 * Attributes used during combat. Available to all the beings.
 */
enum
{
    BASE_ATTR_BEGIN = 0,
    BASE_ATTR_PHY_ATK = BASE_ATTR_BEGIN,
                       /**< Physical attack power. */
    BASE_ATTR_MAG_ATK, /**< Magical attack power. */
    BASE_ATTR_PHY_RES, /**< Resistance to physical damage. */
    BASE_ATTR_MAG_RES, /**< Resistance to magical damage. */
    BASE_ATTR_EVADE,   /**< Ability to avoid hits. */
    BASE_ATTR_HP,      /**< Remaining Hit Points. */
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
 * Please keep weapon skills in sync with item weapon types.
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
    CHAR_ATTR_CHARISMA,
    CHAR_ATTR_END,
    CHAR_ATTR_NB = CHAR_ATTR_END - CHAR_ATTR_BEGIN,

    CHAR_SKILL_WEAPON_BEGIN = CHAR_ATTR_END,
    CHAR_SKILL_WEAPON_NONE = CHAR_SKILL_WEAPON_BEGIN,
    CHAR_SKILL_WEAPON_KNIFE,
    CHAR_SKILL_WEAPON_SWORD,
    CHAR_SKILL_WEAPON_SPEAR,
    CHAR_SKILL_WEAPON_JAVELIN,
    CHAR_SKILL_WEAPON_ROD,
    CHAR_SKILL_WEAPON_STAFF,
    CHAR_SKILL_WEAPON_WHIP,
    CHAR_SKILL_WEAPON_PROJECTILE,
    CHAR_SKILL_WEAPON_BOOMERANG,
    CHAR_SKILL_WEAPON_BOW,
    CHAR_SKILL_WEAPON_SICKLE,
    CHAR_SKILL_WEAPON_CROSSBOW,
    CHAR_SKILL_WEAPON_STICK,
    CHAR_SKILL_WEAPON_HAMMER,
    CHAR_SKILL_WEAPON_AXE,
    CHAR_SKILL_WEAPON_HAND_PROJECTILE,
    CHAR_SKILL_WEAPON_END,
    CHAR_SKILL_WEAPON_NB = CHAR_SKILL_WEAPON_END - CHAR_SKILL_WEAPON_BEGIN,

    // Magic skills should follow.

    NB_CHARACTER_ATTRIBUTES = CHAR_SKILL_WEAPON_END
};

#endif // _TMWSERV_DEFINES_H_
