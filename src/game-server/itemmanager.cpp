/*
 *  The Mana World
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

#include <map>

#include "game-server/itemmanager.hpp"

#include "defines.h"
#include "game-server/item.hpp"
#include "game-server/resourcemanager.hpp"
#include "utils/logger.h"
#include "utils/xml.hpp"

typedef std::map< int, ItemClass * > ItemClasses;
static ItemClasses itemClasses; /**< Item reference */
static std::string itemReferenceFile;


ItemType itemTypeFromString (std::string name, int id = 0)
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
    else if (name=="")
    {
        LOG_WARN("No item type defined for item "<<id<<" in items.xml");
        return ITEM_UNUSABLE;
    }
    else
    {
        LOG_WARN("Unknown item type \""<<name<<"\" for item "<<id<<" in items.xml");
        if (name.find("weapon") != std::string::npos)
            LOG_WARN("do you mean \"equip-1hand\" or \"equip-2hand\"?");
        if (name.find("armor") != std::string::npos)
            LOG_WARN("do you mean \"equip-...\" instead of \"armor-...\"?");
        return ITEM_UNUSABLE;
    }
}

WeaponType weaponTypeFromString (std::string name, int id = 0)
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
    else if (name=="")
    {
        LOG_WARN("ItemManager: No weapon type defined for weapon with item id "<<id);
        return WPNTYPE_NONE;
    }
    else
    {
        LOG_WARN("ItemManager: Unknown weapon type \""<<name<<"\" for item "<<id);
        return WPNTYPE_NONE;
    }
}

void ItemManager::initialize(std::string const &file)
{
    itemReferenceFile = file;
    reload();
}

void ItemManager::reload()
{
    int size;
    char *data = ResourceManager::loadFile(itemReferenceFile, size);

    if (!data) {
        LOG_ERROR("Item Manager: Could not find " << itemReferenceFile << "!");
        free(data);
        return;
    }

    xmlDocPtr doc = xmlParseMemory(data, size);
    free(data);

    if (!doc)
    {
        LOG_ERROR("Item Manager: Error while parsing item database ("
                  << itemReferenceFile << ")!");
        return;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node || !xmlStrEqual(node->name, BAD_CAST "items"))
    {
        LOG_ERROR("Item Manager: " << itemReferenceFile
                  << " is not a valid database file!");
        xmlFreeDoc(doc);
        return;
    }

    LOG_INFO("Loading item reference...");
    unsigned nbItems = 0;
    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
        if (!xmlStrEqual(node->name, BAD_CAST "item"))
        {
            continue;
        }

        int id = XML::getProperty(node, "id", 0);
        if (id == 0)
        {
            LOG_WARN("Item Manager: An (ignored) item has no ID in "
                     << itemReferenceFile << "!");
            continue;
        }

        std::string sItemType = XML::getProperty(node, "type", "");
        ItemType itemType = itemTypeFromString(sItemType, id);

        ItemClass *item;
        ItemClasses::iterator i = itemClasses.find(id);
        if (i == itemClasses.end())
        {
            item = new ItemClass(id, itemType);
            itemClasses[id] = item;
        }
        else
        {
            item = i->second;
        }

        int weight = XML::getProperty(node, "weight", 0);
        int value = XML::getProperty(node, "value", 0);
        int maxPerSlot = XML::getProperty(node, "max-per-slot", 0);
        int sprite = XML::getProperty(node, "sprite_id", 0);
        std::string scriptName = XML::getProperty(node, "script_name", std::string());

        ItemModifiers modifiers;
        if (itemType == ITEM_EQUIPMENT_ONE_HAND_WEAPON ||
            itemType == ITEM_EQUIPMENT_TWO_HANDS_WEAPON)
        {
            std::string sWeaponType = XML::getProperty(node, "weapon-type", "");
            WeaponType weaponType = weaponTypeFromString(sWeaponType, id);
            modifiers.setValue(MOD_WEAPON_TYPE, weaponType);
            modifiers.setValue(MOD_WEAPON_RANGE,  XML::getProperty(node, "range",       0));
            modifiers.setValue(MOD_ELEMENT_TYPE,  XML::getProperty(node, "element",     0));
        }
        modifiers.setValue(MOD_LIFETIME,      XML::getProperty(node, "lifetime", 0) * 10);
        //TODO: add child nodes for these modifiers (additive and factor)
        modifiers.setAttributeValue(BASE_ATTR_PHY_ATK_MIN,      XML::getProperty(node, "attack-min",      0));
        modifiers.setAttributeValue(BASE_ATTR_PHY_ATK_DELTA,      XML::getProperty(node, "attack-delta",      0));
        modifiers.setAttributeValue(BASE_ATTR_HP,      XML::getProperty(node, "hp",      0));
        modifiers.setAttributeValue(BASE_ATTR_PHY_RES, XML::getProperty(node, "defense", 0));
        modifiers.setAttributeValue(CHAR_ATTR_STRENGTH,     XML::getProperty(node, "strength",     0));
        modifiers.setAttributeValue(CHAR_ATTR_AGILITY,      XML::getProperty(node, "agility",      0));
        modifiers.setAttributeValue(CHAR_ATTR_DEXTERITY,    XML::getProperty(node, "dexterity",    0));
        modifiers.setAttributeValue(CHAR_ATTR_VITALITY,     XML::getProperty(node, "vitality",     0));
        modifiers.setAttributeValue(CHAR_ATTR_INTELLIGENCE, XML::getProperty(node, "intelligence", 0));
        modifiers.setAttributeValue(CHAR_ATTR_WILLPOWER,    XML::getProperty(node, "willpower",    0));

        if (maxPerSlot == 0)
        {
            LOG_WARN("Item Manager: Missing max-per-slot property for "
                     "item " << id << " in " << itemReferenceFile << '.');
            maxPerSlot = 1;
        }

        if (itemType > ITEM_USABLE && itemType < ITEM_EQUIPMENT_AMMO &&
            maxPerSlot != 1)
        {
            LOG_WARN("Item Manager: Setting max-per-slot property to 1 for "
                     "equipment " << id << " in " << itemReferenceFile << '.');
            maxPerSlot = 1;
        }

        if (weight == 0)
        {
            LOG_WARN("Item Manager: Missing weight for item "
                     << id << " in " << itemReferenceFile << '.');
            weight = 1;
        }

        item->setWeight(weight);
        item->setCost(value);
        item->setMaxPerSlot(maxPerSlot);
        //item->setScriptName(scriptName);
        item->setModifiers(modifiers);
        item->setSpriteID(sprite ? sprite : id);
        ++nbItems;

        LOG_DEBUG("Item: ID: " << id << ", itemType: " << itemType
                  << ", weight: " << weight << ", value: " << value <<
                  ", scriptName: " << scriptName << ", maxPerSlot: " << maxPerSlot << ".");
    }

    LOG_INFO("Loaded " << nbItems << " items from "
             << itemReferenceFile << ".");

    xmlFreeDoc(doc);
}

void ItemManager::deinitialize()
{
    for (ItemClasses::iterator i = itemClasses.begin(), i_end = itemClasses.end(); i != i_end; ++i)
    {
        delete i->second;
    }
    itemClasses.clear();
}

ItemClass *ItemManager::getItem(int itemId)
{
    ItemClasses::const_iterator i = itemClasses.find(itemId);
    return i != itemClasses.end() ? i->second : NULL;
}
