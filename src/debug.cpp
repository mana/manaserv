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
 */
 
 // This file contains debugging global functions
 
#include "debug.h"

// global debugging flag (set to false in release version)
int debugflag = false;


void debugCatch(int result)
{
    if (!debugflag) {
        return; // break out if we are not debugging    
    }
              
                   
    switch (result)
    {
        case TMW_SUCCESS: // function successful
            return;
            break;
        case TMW_ACCOUNTERROR_NOEXIST: // account does not exist
            // show the programmer a message
            break;
        case TMW_ACCOUNTERROR_BANNED: // account is banned
            // show the programmer a message
            break;
        case TMW_ACCOUNTERROR_ALREADYASSIGNED: // account is in use
            // show the programmer a message (this may signal a logout bug
            break;
        case TMW_ACCOUNTERROR_CHARNOTFOUND: // the character is not found
            // show a message
            break;
        case TMW_ACCOUNTERROR_ASSIGNFAILED: // failed to assign the handle to the user
            // show a message
            break;
    }
}
