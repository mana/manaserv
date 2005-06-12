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

#include "object.h"

Being::Being(const std::string &bName, unsigned int bGender,
        unsigned int bLevel, unsigned int bMoney,
        unsigned int bStrength, unsigned int bAgility,
        unsigned int bVitality, unsigned int bDexterity,
        unsigned int bLuck):
    name(bName),
    gender(bGender),
    level(bLevel),
    money(bMoney),
    strength(bStrength),
    agility(bAgility),
    vitality(bVitality),
    dexterity(bDexterity),
    luck(bLuck)
{
    std::cout << "New being create with name \"" + name + "\"" << std::endl;
}

void Being::update()
{
    //Generate statistics
    stats.health = 20 + (20 * vitality);
    stats.attack = 10 + strength;
    stats.defense = 10 + strength;
    stats.magic = 10 + intelligence;
    stats.accuracy = 50 + dexterity;
    stats.speed = dexterity;

    //Update scipt
#ifdef SCRIPT_SUPPORT
    script->update();
#endif
}




