/*
 *  The Mana Server
 *  Copyright (C) 2008  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UTILS_ENCRYPTION_H
#define UTILS_ENCRYPTION_H

#include <string>

namespace Encryption {

/**
 * Creates a random string, suitable for a user to type,
 * and that doesn't break a database
 */
std::string createRandomPassword();

}

#endif // UTILS_ENCRYPTION_H
