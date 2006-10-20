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
 *  $Id:$
 */

#include "itemmanager.h"

#include "resourcemanager.h"
#include "utils/logger.h"
#include <libxml/tree.h>

#define READ_PROP(node, prop, name, target, cast) \
        prop = xmlGetProp(node, BAD_CAST name); \
        if (prop) { \
            target = cast((const char*)prop); \
            xmlFree(prop); \
        }

ItemManager::ItemManager(const std::string &itemReferenceFile)
{
    ResourceManager *resman = ResourceManager::getInstance();
    int size;
    char *data = (char*)resman->loadFile(itemReferenceFile, size);

    if (!data) {
        LOG_ERROR("Item Manager: Could not find " << itemReferenceFile << "!", 0);
        free(data);
    }
    else
    {
        xmlDocPtr doc = xmlParseMemory(data, size);
        free(data);

        if (!doc)
        {
            LOG_ERROR("Item Manager: Error while parsing item database ("
            << itemReferenceFile << ")!", 0);
        }
        else
        {
            xmlNodePtr node = xmlDocGetRootElement(doc);
            if (!node || !xmlStrEqual(node->name, BAD_CAST "items"))
            {
                LOG_ERROR("Item Manager: " << itemReferenceFile
                << " is not a valid database file!", 0);
            }
            else
            {
                LOG_INFO("Loading item reference...", 0);
                unsigned int nbItems = 0;
                for (node = node->xmlChildrenNode; node != NULL; node = node->next)
                {
                    // Properties
                    unsigned int id = 0;
                    unsigned short itemType = 0;
                    unsigned int weight = 0;
                    unsigned int value = 0;
                    unsigned short maxPerSlot = 0;
                    std::string scriptName = "";
                    Modifiers modifiers;

                    if (!xmlStrEqual(node->name, BAD_CAST "item")) {
                        continue;
                    }

                    xmlChar *prop = NULL;
                    // Properties
                    READ_PROP(node, prop, "id", id, atoi);
                    READ_PROP(node, prop, "type", itemType, atoi);
                    READ_PROP(node, prop, "weight", weight, atoi);
                    READ_PROP(node, prop, "value", value, atoi);
                    READ_PROP(node, prop, "max_per_slot", maxPerSlot, atoi);
                    READ_PROP(node, prop, "script_name", scriptName, );

                    // --- Modifiers
                    // General
                    READ_PROP(node, prop, "element", modifiers.element,
                    (Element)atoi);
                    READ_PROP(node, prop, "lifetime", modifiers.lifetime, atoi);
                    // Raw Statistics
                    READ_PROP(node, prop, "strength",
                    modifiers.rawStats[STAT_STRENGTH], atoi);
                    READ_PROP(node, prop, "agility",
                    modifiers.rawStats[STAT_AGILITY], atoi);
                    READ_PROP(node, prop, "vitality",
                    modifiers.rawStats[STAT_VITALITY], atoi);
                    READ_PROP(node, prop, "intelligence",
                    modifiers.rawStats[STAT_INTELLIGENCE], atoi);
                    READ_PROP(node, prop, "dexterity",
                    modifiers.rawStats[STAT_DEXTERITY], atoi);
                    READ_PROP(node, prop, "luck",
                    modifiers.rawStats[STAT_LUCK], atoi);
                    // Computed Statistics
                    READ_PROP(node, prop, "heat",
                    modifiers.computedStats[STAT_HEAT], atoi);
                    READ_PROP(node, prop, "attack",
                    modifiers.computedStats[STAT_ATTACK], atoi);
                    READ_PROP(node, prop, "defence",
                    modifiers.computedStats[STAT_DEFENCE], atoi);
                    READ_PROP(node, prop, "magic",
                    modifiers.computedStats[STAT_MAGIC], atoi);
                    READ_PROP(node, prop, "accuracy",
                    modifiers.computedStats[STAT_ACCURACY], atoi);
                    READ_PROP(node, prop, "speed",
                    modifiers.computedStats[STAT_SPEED], atoi);
                    // Main Values
                    READ_PROP(node, prop, "hp", modifiers.hp, atoi);
                    READ_PROP(node, prop, "mp", modifiers.mp, atoi);
                    // Equipment
                    READ_PROP(node, prop, "range", modifiers.range, atoi);
                    READ_PROP(node, prop, "weapon_type", modifiers.weaponType,
                      (WeaponType)atoi);
                    // Status effect
                    READ_PROP(node, prop, "status_effect",
                    modifiers.beingStateEffect, (BeingStateEffect)atoi);

                    // Checks
                    if (id != 0)
                    {
                        ItemPtr item(new Item(modifiers, itemType, weight,
                                            value, scriptName, maxPerSlot));
                        mItemReference[id] = item;
                        nbItems++;
                    }

                    if (id == 0)
                    {
                        LOG_WARN("Item Manager: An (ignored) item has no ID in "
                        << itemReferenceFile << "!", 0);
                    }
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
                    ",attack: " << modifiers.computedStats[STAT_ATTACK] <<
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

            } // End if node "items"

            xmlFreeDoc(doc);

        } // End if doc?
    } // End if data?
}

ItemManager::~ItemManager()
{
    mItemReference.clear();
}
