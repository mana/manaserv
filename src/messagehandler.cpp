/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#include "messagehandler.h"
#include "debug.h"

/* recieveMessage
 * This function recieves a message, then sends it to the appropriate handler
 * sub-routine for processing.
 * Preconditions: valid parameters, queue initialized, etc.
 * Postconditions: message successfully processed.
 */ 
void MessageHandler::receiveMessage(NetComputer *computer, MessageIn &message)
{
    int result = 0;
    
    // determine message type
    /* switch(message.type)
     * {
     *     case: TYPE_LOGIN
     *         result = loginMessage(computer, message);
     *         break;
     * }
     */ 
     
     debugCatch(result);
}


/* loginMessage
 * Accepts a login message and interprets it, assigning the proper login
 * Preconditions: The requested handle is not logged in already. 
 *                The requested handle exists. 
 *                The requested handle is not banned or restricted. 
 *                The character profile is valid
 * Postconditions: the player recieves access through a character in the world.
 * Return Value: SUCCESS if the player was successfully assigned the requested char
 *               ERROR on early termination of the routine.
 */ 
int MessageHandler::loginMessage(NetComputer *computer, MessageIn &message)
{
    // Get the handle (account) the player is requesting
    // RETURN TMW_ACCOUNTERROR_NOEXIST if: requested does not handle exist
    // RETURN TMW_ACCOUNTERROR_BANNED if: the handle status is HANDLE_STATUS_BANNED
    // RETURN TMW_ACCOUNTERROR_ALREADYASSIGNED if: the handle is already assigned
    
    // Get the character within that handle that the player is requesting
    // RETURN TMW_ACCOUNTERROR_CHARNOTFOUND if: character not found
    
    // Assign the player to that character
    // RETURN TMW_ACCOUNTERROR_ASSIGNFAILED if: assignment not successful
    
    // return TMW_SUCCESS -- successful exit
}
