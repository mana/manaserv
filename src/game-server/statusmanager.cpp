/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "game-server/statusmanager.hpp"

#include "common/resourcemanager.hpp"
#include "game-server/statuseffect.hpp"
#include "scripting/script.hpp"
#include "utils/logger.h"
#include "utils/xml.hpp"

#include <map>
#include <set>
#include <sstream>

typedef std::map< int, StatusEffect * > StatusEffects;
static StatusEffects statusEffects;
static std::string statusReferenceFile;

void StatusManager::initialize(const std::string &file)
{
    statusReferenceFile = file;
    reload();
}

void StatusManager::reload()
{
    int size;
    // Note: The file is checked for UTF-8 BOM.
    char *data = ResourceManager::loadFile(statusReferenceFile, size, true);

    if (!data) {
        LOG_ERROR("Status Manager: Could not find " << statusReferenceFile << "!");
        free(data);
        return;
    }

    xmlDocPtr doc = xmlParseMemory(data, size);
    free(data);

    if (!doc)
    {
        LOG_ERROR("Status Manager: Error while parsing status database ("
                  << statusReferenceFile << ")!");
        return;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node || !xmlStrEqual(node->name, BAD_CAST "status-effects"))
    {
        LOG_ERROR("Status Manager: " << statusReferenceFile
                  << " is not a valid database file!");
        xmlFreeDoc(doc);
        return;
    }

    LOG_INFO("Loading status reference...");
    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
        if (!xmlStrEqual(node->name, BAD_CAST "status-effect"))
        {
            continue;
        }

        int id = XML::getProperty(node, "id", 0);
        if (id == 0)
        {
            LOG_WARN("Status Manager: An (ignored) Status has no ID in "
                     << statusReferenceFile << "!");
            continue;
        }

        std::string scriptFile = XML::getProperty(node, "script", "");
        //TODO: Get these modifiers
/*
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
*/
        StatusEffect *statusEffect = new StatusEffect(id);
        if (scriptFile != "")
        {
            std::stringstream filename;
            filename << "scripts/status/" << scriptFile;
            if (ResourceManager::exists(filename.str()))       // file exists!
            {
                LOG_INFO("Loading status script: " << filename.str());
                Script *s = Script::create("lua");
                s->loadFile(filename.str());
                statusEffect->setScript(s);
            } else {
                LOG_WARN("Could not find script file \"" << filename.str() << "\" for status #"<<id);
            }
        }
        statusEffects[id] = statusEffect;
    }

    xmlFreeDoc(doc);
}

void StatusManager::deinitialize()
{
    for (StatusEffects::iterator i = statusEffects.begin(), i_end = statusEffects.end(); i != i_end; ++i)
    {
        delete i->second;
    }
    statusEffects.clear();
}

StatusEffect *StatusManager::getStatus(int statusId)
{
    StatusEffects::const_iterator i = statusEffects.find(statusId);
    return i != statusEffects.end() ? i->second : NULL;
}

