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
 */
enum
{
    AL_BANNED =  0,     /**< This user is currently banned. */
    AL_NORMAL = 10,     /**< User has regular rights. */
    AL_GM     = 50,     /**< User can perform a subset of administrator tasks. */
    AL_ADMIN  = 99      /**< User can perform administrator tasks. */
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

    // Post related
    MAX_ATTACHMENTS = 3,
    MAX_LETTERS = 10,

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
    POINTS_TO_DISTRIBUTES_AT_LVL1 = 60,

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
    APMSG_CHAR_INFO                = 0x0024, // B index, S name, B gender, B hair style, B hair color, W level, W character points, W correction points, D money, W*6 stats
    PAMSG_CHAR_SELECT              = 0x0026, // B index
    APMSG_CHAR_SELECT_RESPONSE     = 0x0027, // B error, B*32 token, S game address, W game port, S chat address, W chat port
    PAMSG_EMAIL_CHANGE             = 0x0030, // S email
    APMSG_EMAIL_CHANGE_RESPONSE    = 0x0031, // B error
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
    GPMSG_PLAYER_EXP_CHANGE        = 0x0140, // { B skill, D exp got, D exp needed }*
    GPMSG_LEVELUP                  = 0x0150, // W new level, W character points, W correction points
    GPMSG_LEVEL_PROGRESS           = 0x0151, // B percent completed to next levelup
    PGMSG_RAISE_ATTRIBUTE          = 0x0160, // B attribute
    GPMSG_RAISE_ATTRIBUTE_RESPONSE = 0x0161, // B error
    PGMSG_LOWER_ATTRIBUTE          = 0x0170, // B attribute
    GPMSG_LOWER_ATTRIBUTE_RESPONSE = 0x0171, // B error
    PGMSG_RESPAWN                  = 0x0180, // -
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
    GPMSG_BEING_ATTACK             = 0x0291, // W being id, B direction, B attacktype
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
    GPMSG_NPC_ERROR                = 0x02B8, // B error
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

    // Guild
    PCMSG_GUILD_CREATE                  = 0x0350, // S name
    CPMSG_GUILD_CREATE_RESPONSE         = 0x0351, // B error, W guild, B rights, W channel
    PCMSG_GUILD_INVITE                  = 0x0352, // W id, S name
    CPMSG_GUILD_INVITE_RESPONSE         = 0x0353, // B error
    PCMSG_GUILD_ACCEPT                  = 0x0354, // W id
    CPMSG_GUILD_ACCEPT_RESPONSE         = 0x0355, // B error, W guild, B rights, W channel
    PCMSG_GUILD_GET_MEMBERS             = 0x0356, // W id
    CPMSG_GUILD_GET_MEMBERS_RESPONSE    = 0x0357, // S names, B online
    CPMSG_GUILD_UPDATE_LIST             = 0x0358, // W id, S name, B event
    PCMSG_GUILD_QUIT                    = 0x0360, // W id
    CPMSG_GUILD_QUIT_RESPONSE           = 0x0361, // B error
    PCMSG_GUILD_PROMOTE_MEMBER          = 0x0365, // W guild, S name, B rights
    CPMSG_GUILD_PROMOTE_MEMBER_RESPONSE = 0x0366, // B error

    CPMSG_GUILD_INVITED                 = 0x0370, // S char name, S  guild name, W id
    CPMSG_GUILD_REJOIN                  = 0x0371, // S name, W guild, W rights, W channel, S announce

    // Party
    PCMSG_PARTY_INVITE                  = 0x03A0, // S name
    CPMSG_PARTY_INVITE_RESPONSE         = 0x03A1, // B error, S name
    CPMSG_PARTY_INVITED                 = 0x03A2, // S name
    PCMSG_PARTY_ACCEPT_INVITE           = 0x03A5, // S name
    CPMSG_PARTY_ACCEPT_INVITE_RESPONSE  = 0x03A6, // B error
    PCMSG_PARTY_QUIT                    = 0x03AA, // -
    CPMSG_PARTY_QUIT_RESPONSE           = 0x03AB, // B error

    // Chat
    CPMSG_ERROR                    = 0x0401, // B error
    CPMSG_ANNOUNCEMENT             = 0x0402, // S text
    CPMSG_PRIVMSG                  = 0x0403, // S user, S text
    CPMSG_PUBMSG                   = 0x0404, // W channel, S user, S text
    PCMSG_CHAT                     = 0x0410, // S text, W channel
    PCMSG_ANNOUNCE                 = 0x0411, // S text
    PCMSG_PRIVMSG                  = 0x0412, // S user, S text
    // -- Channeling
    CPMSG_CHANNEL_EVENT               = 0x0430, // W channel, B event, S info
    PCMSG_ENTER_CHANNEL               = 0x0440, // S channel, S password
    CPMSG_ENTER_CHANNEL_RESPONSE      = 0x0441, // B error, W id, S name, S topic, S userlist
    PCMSG_QUIT_CHANNEL                = 0x0443, // W channel id
    CPMSG_QUIT_CHANNEL_RESPONSE       = 0x0444, // B error, W channel id
    PCMSG_LIST_CHANNELS               = 0x0445, // -
    CPMSG_LIST_CHANNELS_RESPONSE      = 0x0446, // S names, W number of users
    PCMSG_LIST_CHANNELUSERS           = 0x0460, // S channel
    CPMSG_LIST_CHANNELUSERS_RESPONSE  = 0x0461, // S channel, { S user, B mode }
    PCMSG_TOPIC_CHANGE                = 0x0462, // W channel id, S topic
    // -- User modes
    PCMSG_USER_MODE                   = 0x0465, // W channel id, S name, B mode
    PCMSG_KICK_USER                   = 0x0466, // W channel id, S name

    // Inter-server
    GAMSG_REGISTER     = 0x0500, // S address, W port, { W map id }*
    AGMSG_ACTIVE_MAP   = 0x0501, // W map id
    AGMSG_PLAYER_ENTER = 0x0510, // B*32 token, L id, S name, serialised character data
    GAMSG_PLAYER_DATA  = 0x0520, // L id, serialised character data
    GAMSG_REDIRECT          = 0x0530, // L id
    AGMSG_REDIRECT_RESPONSE = 0x0531, // L id, B*32 token, S game address, W game port
    GAMSG_PLAYER_RECONNECT  = 0x0532, // L id, B*32 token
    GAMSG_SET_QUEST          = 0x0540, // L id, S name, S value
    GAMSG_GET_QUEST          = 0x0541, // L id, S name
    AGMSG_GET_QUEST_RESPONSE = 0x0542, // L id, S name, S value
    GAMSG_BAN_PLAYER = 0x550, // L id, W duration
    GAMSG_STATISTICS = 0x560, // { W map id, W thing nb, W monster nb, W player nb, { L character id }* }*
    CGMSG_CHANGED_PARTY                 = 0x0590, // L character id, L party id

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
    ERRMSG_EMAIL_ALREADY_EXISTS,        // The Email Address already exists
    ERRMSG_ALREADY_TAKEN,               // name used was already taken
    ERRMSG_SERVER_FULL,                 // the server is overloaded
    ERRMSG_TIME_OUT                     // data failed to arrive in due time
};

// Login specific return values
enum {
    LOGIN_INVALID_VERSION = 0x40,       // the user is using an incompatible protocol
    LOGIN_BANNED                        // the user is currently banned
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

// Character attribute modification specific return value
enum AttribmodResponseCode {
    ATTRIBMOD_OK = ERRMSG_OK,
    ATTRIBMOD_INVALID_ATTRIBUTE = 0x40,
    ATTRIBMOD_NO_POINTS_LEFT,
    ATTRIBMOD_DENIED
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
    CHAT_EVENT_LEAVING_PLAYER,
    CHAT_EVENT_TOPIC_CHANGE,
    CHAT_EVENT_MODE_CHANGE,
    CHAT_EVENT_KICKED_PLAYER
};

// Guild member event values
enum {
    GUILD_EVENT_NEW_PLAYER = 0,
    GUILD_EVENT_LEAVING_PLAYER,
    GUILD_EVENT_ONLINE_PLAYER,
    GUILD_EVENT_OFFLINE_PLAYER
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
    CHAR_ATTR_END,
    CHAR_ATTR_NB = CHAR_ATTR_END - CHAR_ATTR_BEGIN,

    CHAR_SKILL_BEGIN = CHAR_ATTR_END,

    CHAR_SKILL_WEAPON_BEGIN = CHAR_SKILL_BEGIN,
    CHAR_SKILL_WEAPON_NONE = CHAR_SKILL_WEAPON_BEGIN,
    CHAR_SKILL_WEAPON_KNIFE,
    CHAR_SKILL_WEAPON_SWORD,
    CHAR_SKILL_WEAPON_POLEARM,
    CHAR_SKILL_WEAPON_STAFF,
    CHAR_SKILL_WEAPON_WHIP,
    CHAR_SKILL_WEAPON_BOW,
    CHAR_SKILL_WEAPON_SHOOTING,
    CHAR_SKILL_WEAPON_MACE,
    CHAR_SKILL_WEAPON_AXE,
    CHAR_SKILL_WEAPON_THROWN,
    CHAR_SKILL_WEAPON_END,
    CHAR_SKILL_WEAPON_NB = CHAR_SKILL_WEAPON_END - CHAR_SKILL_WEAPON_BEGIN,

    CHAR_SKILL_MAGIC_BEGIN = CHAR_SKILL_WEAPON_END,
    CHAR_SKILL_MAGIC_IAMJUSTAPLACEHOLDER = CHAR_SKILL_MAGIC_BEGIN,
    // add magic skills here
    CHAR_SKILL_MAGIC_END,
    CHAR_SKILL_MAGIC_NB = CHAR_SKILL_MAGIC_END - CHAR_SKILL_MAGIC_BEGIN,

    CHAR_SKILL_CRAFT_BEGIN = CHAR_SKILL_MAGIC_END,
    CHAR_SKILL_CRAFT_IAMJUSTAPLACEHOLDER = CHAR_SKILL_CRAFT_BEGIN,
    // add crafting skills here
    CHAR_SKILL_CRAFT_END,
    CHAR_SKILL_CRAFT_NB = CHAR_SKILL_CRAFT_END - CHAR_SKILL_CRAFT_BEGIN,

    CHAR_SKILL_END = CHAR_SKILL_CRAFT_END,
    CHAR_SKILL_NB = CHAR_SKILL_END - CHAR_SKILL_BEGIN,

    NB_CHARACTER_ATTRIBUTES = CHAR_SKILL_END
};

#endif // _TMWSERV_DEFINES_H_
