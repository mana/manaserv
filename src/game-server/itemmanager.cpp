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

#include "resourcemanager.h"
#include "game-server/itemmanager.hpp"
#include "utils/logger.h"
#include "utils/xml.hpp"

ItemManager::ItemManager(std::string const &itemReferenceFile)
{
    ResourceManager *resman = ResourceManager::getInstance();
    int size;
    char *data = (char *)resman->loadFile(itemReferenceFile, size);

    if (!data) {
        LOG_ERROR("Item Manager: Could not find " << itemReferenceFile << "!", 0);
        free(data);
        return;
    }

    xmlDocPtr doc = xmlParseMemory(data, size);
    free(data);

    if (!doc)
    {
        LOG_ERROR("Item Manager: Error while parsing item database ("
                  << itemReferenceFile << ")!", 0);
        return;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node || !xmlStrEqual(node->name, BAD_CAST "items"))
    {
        LOG_ERROR("Item Manager: " << itemReferenceFile
                  << " is not a valid database file!", 0);
        xmlFreeDoc(doc);
        return;
    }

    LOG_INFO("Loading item reference...", 0);
    unsigned nbItems = 0;
    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
        if (!xmlStrEqual(node->name, BAD_CAST "item")) {
            continue;
        }

        unsigned id = XML::getProperty(node, "id", 0);

        if (id == 0)
        {
            LOG_WARN("Item Manager: An (ignored) item has no ID in "
                     << itemReferenceFile << "!", 0);
            continue;
        }

        int itemType = XML::getProperty(node, "type", 0);
        int weight = XML::getProperty(node, "weight", 0);
        int value = XML::getProperty(node, "value", 0);
        int maxPerSlot = XML::getProperty(node, "max_per_slot", 0);
        std::string scriptName = XML::getProperty(node, "script_name", std::string());

        Modifiers modifiers;
        modifiers.element = XML::getProperty(node, "element", 0);
        modifiers.lifetime = XML::getProperty(node, "lifetime", 0);
        modifiers.rawStats[STAT_STRENGTH]     = XML::getProperty(node, "strength",     0);
        modifiers.rawStats[STAT_AGILITY]      = XML::getProperty(node, "agility",      0);
        modifiers.rawStats[STAT_VITALITY]     = XML::getProperty(node, "vitality",     0);
        modifiers.rawStats[STAT_INTELLIGENCE] = XML::getProperty(node, "intelligence", 0);
        modifiers.rawStats[STAT_DEXTERITY]    = XML::getProperty(node, "dexterity",    0);
        modifiers.rawStats[STAT_LUCK]         = XML::getProperty(node, "luck",         0);
        modifiers.computedStats[STAT_HEAT]     = XML::getProperty(node, "heat",     0);
        modifiers.computedStats[STAT_ATTACK]   = XML::getProperty(node, "attack",   0);
        modifiers.computedStats[STAT_DEFENCE]  = XML::getProperty(node, "defence",  0);
        modifiers.computedStats[STAT_MAGIC]    = XML::getProperty(node, "magic",    0);
        modifiers.computedStats[STAT_ACCURACY] = XML::getProperty(node, "accuracy", 0);
        modifiers.computedStats[STAT_SPEED]    = XML::getProperty(node, "speed",    0);
        modifiers.hp = XML::getProperty(node, "hp", 0);
        modifiers.mp = XML::getProperty(node, "mp", 0);
        modifiers.range = XML::getProperty(node, "range", 0);
        modifiers.weaponType = XML::getProperty(node, "weapon_type", 0);
        modifiers.beingStateEffect = XML::getProperty(node, "status_effect", 0);

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
                     << id << " in " << itemReferenceFile << ".", 0);
        }
        if (weight == 0)
        {
            LOG_WARN("Item Manager: Missing weight for item: "
                     << id << " in " << itemReferenceFile << ".", 0);
        }

        LOG_INFO("Item: ID: " << id << ", itemType: " << itemType
                 << ", weight: " << weight << ", value: " << value <<
                 ", scriptName: " << scriptName << ", maxPerSlot: " << maxPerSlot << ".", 3);
        // Log level 5
        LOG_INFO("Modifiers:: element: " <<  modifiers.element <<
                 ", lifetime: " << modifiers.lifetime
                 << std::endl <<
                 ", strength: " << modifiers.rawStats[STAT_STRENGTH] <<
                 ", agility: " << modifiers.rawStats[STAT_AGILITY] <<
                 ", vitality: " << modifiers.rawStats[STAT_VITALITY]
                 << std::endl <<
                 ", intelligence: " << modifiers.rawStats[STAT_INTELLIGENCE] <<
                 ", dexterity: " << modifiers.rawStats[STAT_DEXTERITY] <<
                 ", luck: " << modifiers.rawStats[STAT_LUCK]
                 << std::endl <<
                 ", heat: " << modifiers.computedStats[STAT_HEAT] <<
                 ", attack: " << modifiers.computedStats[STAT_ATTACK] <<
                 ", defence: " << modifiers.computedStats[STAT_DEFENCE]
                 << std::endl <<
                 ", magic: " << modifiers.computedStats[STAT_MAGIC] <<
                 ", accuracy: " << modifiers.computedStats[STAT_ACCURACY] <<
                 ", speed: " << modifiers.computedStats[STAT_SPEED] <<
                 std::endl <<
                 ", hp: " << modifiers.hp <<
                 ", mp: " << modifiers.mp <<
                 std::endl <<
                 ", range: " << modifiers.range <<
                 ", weapon_type: " << modifiers.weaponType <<
                 ", status_effect: " << modifiers.beingStateEffect, 5);
    }

    LOG_INFO("Loaded " << nbItems << " items from "
             << itemReferenceFile << ".", 0);

    xmlFreeDoc(doc);
}

ItemClass *ItemManager::getItem(int itemId) const
{
    std::map< int, ItemClass * >::const_iterator i = mItemReference.find(itemId);
    return i != mItemReference.end() ? i->second : NULL;
}
