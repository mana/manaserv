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

#include "game-server/skillmanager.hpp"

#include "common/resourcemanager.hpp"
#include "utils/string.hpp"   // for the toupper function
#include "utils/logger.h"
#include "utils/xml.hpp"

#include <map>

typedef std::map< std::string, int > SkillMap;
static SkillMap skillMap;
static std::string skillReferenceFile;

void SkillManager::initialize(const std::string &file)
{
    skillReferenceFile = file;
    reload();
}

void SkillManager::reload()
{
    /*
    skillMap["UNARMED"] = 100;
    skillMap["KNIFE"] = 101;
    */

    int size;
    // Note: The file is checked for UTF-8 BOM.
    char *data = ResourceManager::loadFile(skillReferenceFile, size, true);

    if (!data) {
        LOG_ERROR("Item Manager: Could not find " << skillReferenceFile << "!");
        free(data);
        return;
    }

    xmlDocPtr doc = xmlParseMemory(data, size);
    free(data);

    if (!doc)
    {
        LOG_ERROR("Skill Manager: Error while parsing skill database ("
                  << skillReferenceFile << ")!");
        return;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node || !xmlStrEqual(node->name, BAD_CAST "skills"))
    {
        LOG_ERROR("Skill Manager: " << skillReferenceFile
                  << " is not a valid database file!");
        xmlFreeDoc(doc);
        return;
    }

    LOG_INFO("Loading skill reference...");

    for_each_xml_child_node(setnode, node)
    {
        if (xmlStrEqual(setnode->name, BAD_CAST "set"))
        // we don't care about sets server-sided (yet?)
        for_each_xml_child_node(skillnode, setnode)
        {
            if (xmlStrEqual(skillnode->name, BAD_CAST "skill"))
            {
                std::string name = XML::getProperty(skillnode, "name", std::string());
                name = utils::toupper(name);
                int id = XML::getProperty(skillnode, "id", 0);
                if (id && !name.empty())
                {
                    skillMap[utils::toupper(name)] = id;
                }
            }
        }
    }

    LOG_DEBUG("skill map:");
    for (SkillMap::iterator i = skillMap.begin(); i != skillMap.end(); i++)
    {
        LOG_DEBUG("  "<<i->first<<" : "<<i->second);
    }
}

int SkillManager::getIdFromString(std::string name)
{
    //check if already an integer, if yes just return it
    int val;
    val = atoi(name.c_str());
    if (val) return val;

    // convert to upper case for easier finding
    name = utils::toupper(name);
    // find it
    SkillMap::iterator i = skillMap.find(name);
    if (i == skillMap.end())
    {
        return 0;
    } else {
        return i->second;
    }
}
