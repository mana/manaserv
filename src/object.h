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


#ifndef OBJECT_H
#define OBJECT_H

#include <iostream>
#include "script.h"

//Usable Statistics Definition (not unsigned to allow for negative; used for
//summative statistics)
struct Statistics
{
    int health;
    int attack;
    int defense;
    int magic;
    int accuracy;
    int speed;
};

/*
 * Generic In-Game Object Definition
 * Base class for in-game objects
 */
class Object
{
    int x;
    int y;
  public:
    virtual ~Object() { }
    virtual void update() = 0;
};

/*
 * Generic Being (Living Object)
 * Used for Player & Monster (all animate objects)
 */
class Being : public Object
{
    //Being name
    std::string name;

    //Being gender
    unsigned int gender;

    //Being level
    unsigned int level;

    //Being money
    unsigned int money;

    //Being statistics
    unsigned int strength;
    unsigned int agility;
    unsigned int vitality;
    unsigned int intelligence;
    unsigned int dexterity;
    unsigned int luck;

    //Derived statistics (derived from above)
    Statistics stats;

    //Being inventory/equiped items
    //Inventory inventory;
    //Equipment equipment;

#ifdef SCRIPT_SUPPORT
    Script *script;
#endif

    // disable default constructor (we don't want uninitialized Being's)

  public:
    Being() { };

    Being(const std::string &bName, unsigned int bGender,
          unsigned int bLevel, unsigned int bMoney,
          unsigned int bStrength, unsigned int bAgility,
          unsigned int bVitality, unsigned int bDexterity,
          unsigned int bLuck);

    virtual ~Being() { } //empty definition

    //update 
    void update();

    //accessors
    const std::string& getName() { return name; }
    unsigned int getGender() { return gender; }
    unsigned int getLevel() { return level; }
    unsigned int getMoney() { return money; }

    //Get statistics information
    const Statistics& getStatistics() { return stats; }
};

#endif
