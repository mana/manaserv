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
 *  $Id: $
 */

#include "inventory.h"

unsigned short
getSlotIndex(const unsigned int itemId)
{
    return 0;
}

bool
addItem(unsigned int itemId, unsigned short amount)
{
    return false;
}

bool
removeItem(unsigned int itemId, unsigned short amount = 0)
{
    return false;
}

bool
removeItem(unsigned short index, unsigned short amount = 0)
{
    return false;
}

bool
equipItem(unsigned int itemId)
{
    return false;
}

bool
unequipItem(unsigned int itemId)
{
    return false;
}

bool
equipItem(unsigned short index)
{
    return false;
}

bool
unequipItem(unsigned short index)
{
    return false;
}


bool
use(unsigned short index, BeingPtr itemUser)
{
    return false;
}

bool
use(unsigned int itemId, BeingPtr itemUser)
{
    return false;
}

bool
useWithScript(unsigned short index, const std::string scriptFile)
{
    return false;
}

bool
useWithScript(unsigned int itemId, const std::string scriptFile)
{
    return false;
}
