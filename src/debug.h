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

#ifndef _TMW_SERVER_DEBUG_
#define _TMW_SERVER_DEBUG_
 
// This file defines the return types for debugging

/**
 * Returns a message on function failure if the debug flag is set to true.
 */ 
extern void debugCatch(int result);


// message handler definitions
// add your definitions to this list, sorted by type. Each group starts with a
// different multiple of 100

 // GENERAL
#define TMW_SUCCESS                              1 // the function completed successfully
 
 // ACCOUNT
#define TMW_ACCOUNTERROR_NOEXIST               100
#define TMW_ACCOUNTERROR_BANNED                101
#define TMW_ACCOUNTERROR_ALREADYASSIGNED       102
#define TMW_ACCOUNTERROR_CHARNOTFOUND          103
#define TMW_ACCOUNTERROR_ASSIGNFAILED          104

#endif
