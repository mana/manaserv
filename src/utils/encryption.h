/*
 *  The Mana World
 *  Copyright 2008 The Mana World Development Team
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
 *  $Id: $
 */

#ifndef _TMW_UTILS_ENCRYPTION_H
#define _TMW_UTILS_ENCRYPTION_H

#include <string>

#include "sha2.h"

namespace Encryption {

/** SHA256 Encryption related */
const char SHA256HashLength = 64;

/** Set the encryption strength */
const sha2::SHA_TYPE shaType = sha2::enuSHA256;

/** Create an SHA2 hash from a given string */
std::string GetSHA2Hash(std::string stringToHash);

/** Create a random string, suitable for a user to type,
  * and that doesn't break a database */
std::string CreateRandomPassword();

}

#endif //TMW_UTILS_ENCRYPTION_H

