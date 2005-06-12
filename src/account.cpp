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

#include "account.h"

Account::Account()
{
}

Account::Account(const std::string &name, const std::string &password,
        const std::string &email, const std::vector<Being*> &beings):
    name(name),
    password(password),
    email(email),
    beings(beings)
{
}

Account::~Account()
{
}

const std::string& Account::getName()
{
    return name;
}

const std::string& Account::getPassword()
{
    return password;
}

const std::string& Account::getEmail()
{
    return email;
}

Being* Account::getCharacter(const std::string &name)
{
    for (unsigned int i = 0; i < beings.size(); i++)
    {
        if (beings[i]->getName() == name)
        {
            return beings[i];
        }
    }

    return NULL;
}

void Account::setName(const std::string &name)
{
    this->name = name;
}

void Account::setPassword(const std::string &password)
{
    // A hash of p needs to be made then hash stored in password
    this->password = password;
}

void Account::setEmail(const std::string &email)
{
    this->email = email;
}

void Account::setCharacters(const std::vector<Being*> &beings)
{
    this->beings = beings;
}
