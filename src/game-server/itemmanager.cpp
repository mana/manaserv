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
        int itemType = XML::getProperty(node, "type", 0);

        if (id == 0)
        {
            LOG_WARN("Item Manager: An (ignored) item has no ID in "
                     << itemReferenceFile << "!");
            continue;
        }

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
        int maxPerSlot = XML::getProperty(node, "max_per_slot", 0);
        int sprite = XML::getProperty(node, "sprite_id", 0);
        std::string scriptName = XML::getProperty(node, "script_name", std::string());

        //TODO: add child nodes for these modifiers (additive and factor)
        ItemModifiers modifiers;
        modifiers.setValue(MOD_WEAPON_TYPE,   XML::getProperty(node, "weapon_type", 0));
        modifiers.setValue(MOD_WEAPON_RANGE,  XML::getProperty(node, "range",       0));
        modifiers.setValue(MOD_WEAPON_DAMAGE, XML::getProperty(node, "attack",      0));
        modifiers.setValue(MOD_ELEMENT_TYPE,  XML::getProperty(node, "element",     0));
        modifiers.setValue(MOD_LIFETIME,      XML::getProperty(node, "lifetime", 0) * 10);
        modifiers.setAttributeValue(BASE_ATTR_HP,      XML::getProperty(node, "hp",      0));
        modifiers.setAttributeValue(BASE_ATTR_PHY_RES, XML::getProperty(node, "defense", 0));
        modifiers.setAttributeValue(CHAR_ATTR_STRENGTH,     XML::getProperty(node, "strength",     0));
        modifiers.setAttributeValue(CHAR_ATTR_AGILITY,      XML::getProperty(node, "agility",      0));
        modifiers.setAttributeValue(CHAR_ATTR_DEXTERITY,    XML::getProperty(node, "dexterity",    0));
        modifiers.setAttributeValue(CHAR_ATTR_VITALITY,     XML::getProperty(node, "vitality",     0));
        modifiers.setAttributeValue(CHAR_ATTR_INTELLIGENCE, XML::getProperty(node, "intelligence", 0));
        modifiers.setAttributeValue(CHAR_ATTR_WILLPOWER,    XML::getProperty(node, "willpower",    0));
        modifiers.setAttributeValue(CHAR_ATTR_CHARISMA,     XML::getProperty(node, "charisma",     0));

        if (maxPerSlot == 0)
        {
            LOG_WARN("Item Manager: Missing max_per_slot property for "
                     "item " << id << " in " << itemReferenceFile << '.');
            maxPerSlot = 1;
        }

        if (itemType > ITEM_USABLE && itemType < ITEM_EQUIPMENT_PROJECTILE &&
            maxPerSlot != 1)
        {
            LOG_WARN("Item Manager: Setting max_per_slot property to 1 for "
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
