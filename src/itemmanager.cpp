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

ItemManager::ItemManager(std::string itemReferenceFile)
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

            unsigned int nbItems = 0;
            for (node = node->xmlChildrenNode; node != NULL; node = node->next)
            {
                // Properties
                unsigned int id = 0;
                unsigned short itemType = 0;
                unsigned int weight = 0;
                unsigned int value = 0;
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
                READ_PROP(node, prop, "script_name", scriptName, );

                // --- Modifiers
                // General
                READ_PROP(node, prop, "element", modifiers.element, (Element)atoi);
                READ_PROP(node, prop, "lifetime", modifiers.lifetime, atoi);
                // Raw Statistics
                READ_PROP(node, prop, "strength", modifiers.rawStats[STAT_STRENGTH], atoi);
                READ_PROP(node, prop, "agility", modifiers.rawStats[STAT_AGILITY], atoi);
                READ_PROP(node, prop, "vitality", modifiers.rawStats[STAT_VITALITY], atoi);
                READ_PROP(node, prop, "intelligence", modifiers.rawStats[STAT_INTELLIGENCE], atoi);
                READ_PROP(node, prop, "dexterity", modifiers.rawStats[STAT_DEXTERITY], atoi);
                READ_PROP(node, prop, "luck", modifiers.rawStats[STAT_LUCK], atoi);
                // Computed Statistics
                READ_PROP(node, prop, "heat", modifiers.computedStats[STAT_HEAT], atoi);
                READ_PROP(node, prop, "attack", modifiers.computedStats[STAT_ATTACK], atoi);
                READ_PROP(node, prop, "defence", modifiers.computedStats[STAT_DEFENCE], atoi);
                READ_PROP(node, prop, "magic", modifiers.computedStats[STAT_MAGIC], atoi);
                READ_PROP(node, prop, "accuracy", modifiers.computedStats[STAT_ACCURACY], atoi);
                READ_PROP(node, prop, "speed", modifiers.computedStats[STAT_SPEED], atoi);
                // Main Values
                READ_PROP(node, prop, "hp", modifiers.hpMod, atoi);
                READ_PROP(node, prop, "mp", modifiers.mpMod, atoi);
                // Equipment
                READ_PROP(node, prop, "range", modifiers.range, atoi);
                // Status effects addition
                READ_PROP(node, prop, "status_normal",
                modifiers.beingStateEffects.STATE_NORMAL, (bool)atoi);
                READ_PROP(node, prop, "status_poisoned",
                modifiers.beingStateEffects.STATE_POISONED, (bool)atoi);
                READ_PROP(node, prop, "status_stoned",
                modifiers.beingStateEffects.STATE_STONED, (bool)atoi);
                READ_PROP(node, prop, "status_stunned",
                modifiers.beingStateEffects.STATE_STUNNED, (bool)atoi);
                READ_PROP(node, prop, "status_slowed",
                modifiers.beingStateEffects.STATE_SLOWED, (bool)atoi);
                READ_PROP(node, prop, "status_tired",
                modifiers.beingStateEffects.STATE_TIRED, (bool)atoi);
                READ_PROP(node, prop, "status_mad",
                modifiers.beingStateEffects.STATE_MAD, (bool)atoi);
                READ_PROP(node, prop, "status_berserk",
                modifiers.beingStateEffects.STATE_BERSERK, (bool)atoi);
                READ_PROP(node, prop, "status_hasted",
                modifiers.beingStateEffects.STATE_HASTED, (bool)atoi);
                READ_PROP(node, prop, "status_floating",
                modifiers.beingStateEffects.STATE_FLOATING, (bool)atoi);
                // Status Effects deletion
                READ_PROP(node, prop, "status_not_poisoned",
                modifiers.beingStateEffects.STATE_NOT_POISONED, (bool)atoi);
                READ_PROP(node, prop, "status_not_stoned",
                modifiers.beingStateEffects.STATE_NOT_STONED, (bool)atoi);
                READ_PROP(node, prop, "status_not_stunned",
                modifiers.beingStateEffects.STATE_NOT_STUNNED, (bool)atoi);
                READ_PROP(node, prop, "status_not_slowed",
                modifiers.beingStateEffects.STATE_NOT_SLOWED, (bool)atoi);
                READ_PROP(node, prop, "status_not_tired",
                modifiers.beingStateEffects.STATE_NOT_TIRED, (bool)atoi);
                READ_PROP(node, prop, "status_not_mad",
                modifiers.beingStateEffects.STATE_NOT_MAD, (bool)atoi);
                READ_PROP(node, prop, "status_not_berserk",
                modifiers.beingStateEffects.STATE_NOT_BERSERK, (bool)atoi);
                READ_PROP(node, prop, "status_not_hasted",
                modifiers.beingStateEffects.STATE_NOT_HASTED, (bool)atoi);
                READ_PROP(node, prop, "status_not_floating",
                modifiers.beingStateEffects.STATE_NOT_FLOATING, (bool)atoi);

                // Checks
                if (id != 0)
                {
                    ItemPtr item(new Item(modifiers, itemType, weight, value, scriptName));
                    mItemReference[id] = item;
                    nbItems++;
                }

                if (id == 0)
                {
                    LOG_WARN("Item Manager: An (ignored) item has no ID in "
                    << itemReferenceFile << "!", 0);
                }
                if (itemType == 0)
                {
                    LOG_WARN("Item Manager: Missing Item Type for item: "
                    << id << " in " << itemReferenceFile << ".", 0);
                }
                if (weight == 0)
                {
                    LOG_WARN("Item Manager: Missing weight for item: "
                    << id << " in " << itemReferenceFile << ".", 0);
                }

                LOG_INFO("Item: ID: " << id << ", itemType: " << itemType
                << ", weight: " << weight << ", value: " << value <<
                ", scriptName: " << scriptName << ".", 3);
                //TODO: Log level 5 with everything
            }

            LOG_INFO("Loaded " << nbItems << " items from " << itemReferenceFile << ".", 0);

            xmlFreeDoc(doc);
        } // End if doc?
    } // End if data?
}

ItemManager::~ItemManager()
{
    mItemReference.clear();
}
