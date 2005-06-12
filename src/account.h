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

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>
#include <vector>
#include "object.h"

#define ACC_MAX_CHARS 4

/**
 * A player account.
 */
class Account
{
    public:
        Account();
        Account(const std::string &name, const std::string &password,
                const std::string &email, const std::vector<Being*> &beings);
        ~Account();

        void setName(const std::string &name);
        void setPassword(const std::string &password);
        void setEmail(const std::string &email);
        void setCharacters(const std::vector<Being*> &beings);

        const std::string& getEmail();
        const std::string& getPassword();
        const std::string& getName();
        Being* getCharacter(const std::string &name);

    private:
        std::string name;              /**< Username */
        std::string password;          /**< Password (md5 hash) */
        std::string email;             /**< Email address */
        std::vector<Being*> beings;    /**< Player data */
};

#endif
