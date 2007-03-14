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

#include "game-server/itemmanager.hpp"

#include "defines.h"
#include "resourcemanager.h"
#include "utils/logger.h"
#include "utils/xml.hpp"

ItemManager::ItemManager(std::string const &itemReferenceFile)
{
    ResourceManager *resman = ResourceManager::getInstance();
    int size;
    char *data = (char *)resman->loadFile(itemReferenceFile, size);

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

        unsigned id = XML::getProperty(node, "id", 0);

        if (id == 0)
        {
            LOG_WARN("Item Manager: An (ignored) item has no ID in "
                     << itemReferenceFile << "!");
            continue;
        }

        int itemType = XML::getProperty(node, "type", 0);
        int weight = XML::getProperty(node, "weight", 0);
        int value = XML::getProperty(node, "value", 0);
        int maxPerSlot = XML::getProperty(node, "max_per_slot", 0);
        std::string scriptName = XML::getProperty(node, "script_name", std::string());

        //TODO: add child nodes for these modifiers (additive and factor)
        Modifiers modifiers;
        modifiers.element = XML::getProperty(node, "element", 0);
        modifiers.lifetime = XML::getProperty(node, "lifetime", 0);
        modifiers.baseAttributes[ATT_STRENGTH]     = XML::getProperty(node, "strength",     0);
        modifiers.baseAttributes[ATT_AGILITY]      = XML::getProperty(node, "agility",      0);
        modifiers.baseAttributes[ATT_VITALITY]     = XML::getProperty(node, "vitality",     0);
        modifiers.baseAttributes[ATT_INTELLIGENCE] = XML::getProperty(node, "intelligence", 0);
        modifiers.baseAttributes[ATT_DEXTERITY]    = XML::getProperty(node, "dexterity",    0);
        modifiers.baseAttributes[ATT_LUCK]    = XML::getProperty(node, "luck",    0);
/**        modifiers.baseAttributes[ATT_WILLPOWER]         = XML::getProperty(node, "willpower",         0);
        modifiers.baseAttributes[ATT_CHARISMA]         = XML::getProperty(node, "charisma",         0);*/
        modifiers.derivedAttributes[ATT_HP_MAXIMUM]     = XML::getProperty(node, "hp",     0);
        modifiers.derivedAttributes[ATT_PHYSICAL_ATTACK_MINIMUM]   = XML::getProperty(node, "attack",   0);
        modifiers.derivedAttributes[ATT_PHYSICAL_DEFENCE]  = XML::getProperty(node, "defence",  0);
        modifiers.derivedAttributes[ATT_MAGIC]    = XML::getProperty(node, "magic",    0);
        modifiers.derivedAttributes[ATT_ACCURACY] = XML::getProperty(node, "accuracy", 0);
        modifiers.derivedAttributes[ATT_SPEED]    = XML::getProperty(node, "speed",    0);
/**        modifiers.hp = XML::getProperty(node, "hp", 0);
        modifiers.mp = XML::getProperty(node, "mp", 0);*/
        modifiers.range = XML::getProperty(node, "range", 0);
        modifiers.weaponType = XML::getProperty(node, "weapon_type", 0);
/**        modifiers.beingStateEffect = XML::getProperty(node, "status_effect", 0);*/

        ItemClass *item = new ItemClass(id, itemType);
        item->setWeight(weight);
        item->setCost(value);
        item->setMaxPerSlot(maxPerSlot);
        item->setScriptName(scriptName);
        item->setModifiers(modifiers);
        mItemReference[id] = item;
        ++nbItems;

        if (maxPerSlot == 0)
        {
            LOG_WARN("Item Manager: Missing max per slot properties for item: "
                     << id << " in " << itemReferenceFile << ".");
        }
        if (weight == 0)
        {
            LOG_WARN("Item Manager: Missing weight for item: "
                     << id << " in " << itemReferenceFile << ".");
        }

        LOG_DEBUG("Item: ID: " << id << ", itemType: " << itemType
                  << ", weight: " << weight << ", value: " << value <<
                  ", scriptName: " << scriptName << ", maxPerSlot: " << maxPerSlot << ".");
    }

    LOG_INFO("Loaded " << nbItems << " items from "
             << itemReferenceFile << ".");

    xmlFreeDoc(doc);
}

ItemClass *ItemManager::getItem(int itemId) const
{
    std::map< int, ItemClass * >::const_iterator i = mItemReference.find(itemId);
    return i != mItemReference.end() ? i->second : NULL;
}
