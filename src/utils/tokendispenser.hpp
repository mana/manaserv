/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
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
 */

#ifndef TOKENDISPENSER_HPP
#define TOKENDISPENSER_HPP

#define MAGIC_TOKEN_LENGTH 32

#include <string>

namespace utils
{
    /**
     * \brief Returns a magic_token.
     *
     * The tokens are used for spanning a user's session across multiple
     * servers.
     * NOTE: Uniqueness is not guaranteed, store the account- or characterId
     *       with the token if that is an issue.
     * NOTE: Not passed-by-reference by design.
     * NOTE: Store the token in a variable in this namespace if you want to
     *       avoid 1 copy operation per use.
     */
    std::string getMagicToken();

} // namespace utils

#endif // TOKENDISPENSER_HPP
