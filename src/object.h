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

/*
 * Generic In-Game Object Definition
 */
class Object
{
    int x;
    int y;
  public:
    ~Object() { }
    virtual void update() = 0;
};

/*
 * Generic Living Object
 */
class LivingObject : public Object
{
    //Object name
    std::string name;

    //Object level
    unsigned int level;

    //Object money
    unsigned int money;

    //Object statistics
    unsigned int strength;
    unsigned int agility;
    unsigned int vitality;
    unsigned int intelligence;
    unsigned int dexterity;
    unsigned int luck;

    //Object inventory/equiped items
    //Inventory inventory;
    //Equipment equipment;

  public:
    ~LivingObject() { };
    void update() { };
};

/*
 * Player object
 */
class Player : public LivingObject
{
    //Player gender (male = true, female = false)
    bool gender;
  public:
};
