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

#include <iostream>
#include "object.h"

#define ACC_MAX_CHARS 4

//Account definition
class Account
{
    //Account name
    std::string name;
    //Account password (MD5 hash)
    std::string password;

    //Filename of this account
    std::string filename;

    //Player data
    Player player[ACC_MAX_CHARS];

  public:
    ~Account() { save(); }
    //Load account from file
    void load(const std::string&);
    //Save account
    void save();
};
