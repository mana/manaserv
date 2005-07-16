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
    CMSG_REGISTER = 0,
    CMSG_ENCRYPTED_REGISTER,
    SMSG_REGISTER_RESPONSE,
    CMSG_LOGIN,
    CMSG_ENCRYPTED_LOGIN,
    SMSG_LOGIN_ERROR,
    SMSG_LOGIN_CONFIRM,
    CMSG_CHAR_CREATE,
    SMSG_CHAR_CREATE_RESPONSE,

    // Objects
    SMSG_NEW_OBJECT = 20,
    SMSG_REMOVE_OBJECT,
    SMSG_CHANGE_OBJECT,
    CMSG_PICKUP,
    CMSG_USER_OBJECT,

    // Beings
    SMSG_NEW_BEING = 30,
    SMSG_REMOVE_BEING,
    SMSG_INVENTORY_UPD,
    SMSG_EQUIPMENT_UPD,
    SMSG_ATTACK,
    SMSG_PATH,
    CMSG_TARGET,
    CMSG_WALK,
    CMSG_START_TRADE,
    CMSG_START_TALK,
    CMSG_REQ_TRADE,

    // Items
    CMSG_USE_ITEM = 40,
    CMSG_EQUIP,

    // Chat
    SMSG_CHAT = 60,
    SMSG_SYSTEM,
    SMSG_ANNOUNCEMENT,
    CMSG_SAY,
    CMSG_ANNOUNCE,
};



#endif // _TMWSERV_DEFINES_H_
