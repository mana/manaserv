/*
 *  The Mana World
 *  Copyright 2006 The Mana World Development Team
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

#include <cstdlib>

#include "utils/xml.hpp"

namespace XML
{

int getProperty(xmlNodePtr node, char const *name, int def)
{
    if (xmlChar *prop = xmlGetProp(node, BAD_CAST name))
    {
        int ret = atoi((char*)prop);
        xmlFree(prop);
        return ret;
    }
    return def;
}

double
getFloatProperty(xmlNodePtr node, const char* name, double def)
{
    double &ret = def;

    xmlChar *prop = xmlGetProp(node, BAD_CAST name);
    if (prop) {
        ret = atof((char*)prop);
        xmlFree(prop);
    }

    return ret;
}

std::string getProperty(xmlNodePtr node, char const *name, std::string const &def)
{
    if (xmlChar *prop = xmlGetProp(node, BAD_CAST name))
    {
        std::string val = (char *)prop;
        xmlFree(prop);
        return val;
    }
    return def;
}

unsigned int elementFromString(std::string name)
{
    if      (name=="neutral")   return ELEMENT_NEUTRAL;
    else if (name=="fire")      return ELEMENT_FIRE;
    else if (name=="water")     return ELEMENT_WATER;
    else if (name=="earth")     return ELEMENT_EARTH;
    else if (name=="air")       return ELEMENT_AIR;
    else if (name=="lightning") return ELEMENT_LIGHTNING;
    else if (name=="metal")     return ELEMENT_METAL;
    else if (name=="wood")      return ELEMENT_WOOD;
    else if (name=="ice")       return ELEMENT_ICE;
    {
        return ELEMENT_ILLEGAL;
    }
}

ItemType itemTypeFromString (std::string name)
{
    if      (name=="generic")           return ITEM_UNUSABLE;
    else if (name=="usable")            return ITEM_USABLE;
    else if (name=="equip-1hand")       return ITEM_EQUIPMENT_ONE_HAND_WEAPON;
    else if (name=="equip-2hand")       return ITEM_EQUIPMENT_TWO_HANDS_WEAPON;
    else if (name=="equip-torso")       return ITEM_EQUIPMENT_TORSO;
    else if (name=="equip-arms")        return ITEM_EQUIPMENT_ARMS;
    else if (name=="equip-head")        return ITEM_EQUIPMENT_HEAD;
    else if (name=="equip-legs")        return ITEM_EQUIPMENT_LEGS;
    else if (name=="equip-shield")      return ITEM_EQUIPMENT_SHIELD;
    else if (name=="equip-ring")        return ITEM_EQUIPMENT_RING;
    else if (name=="equip-necklace")    return ITEM_EQUIPMENT_NECKLACE;
    else if (name=="equip-feet")        return ITEM_EQUIPMENT_FEET;
    else if (name=="equip-ammo")        return ITEM_EQUIPMENT_AMMO;
    else if (name=="hairsprite")        return ITEM_HAIRSPRITE;
    else if (name=="racesprite")        return ITEM_RACESPRITE;
    else
    {
        return ITEM_UNKNOWN;
    }
}

WeaponType weaponTypeFromString (std::string name)
{
    if      (name=="knife")      return WPNTYPE_KNIFE;
    else if (name=="sword")      return WPNTYPE_SWORD;
    else if (name=="polearm")    return WPNTYPE_POLEARM;
    else if (name=="staff")      return WPNTYPE_STAFF;
    else if (name=="whip")       return WPNTYPE_WHIP;
    else if (name=="bow")        return WPNTYPE_BOW;
    else if (name=="shooting")   return WPNTYPE_SHOOTING;
    else if (name=="mace")       return WPNTYPE_MACE;
    else if (name=="axe")        return WPNTYPE_AXE;
    else if (name=="thrown")     return WPNTYPE_THROWN;
    else
    {
        return WPNTYPE_NONE;
    }
}


} // namespace XML
