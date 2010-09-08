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
#include "utils/string.hpp"   // for the toUpper function
#include "utils/logger.h"
#include "utils/xml.hpp"

#include <map>

typedef std::map< std::string, int > SkillMap;
static SkillMap skillMap;
static std::string skillReferenceFile;
static std::string defaultSkillKey = std::string();

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
    char *data = ResourceManager::loadFile(skillReferenceFile, size);

    std::string absPathFile = ResourceManager::resolve(skillReferenceFile);

    if (!data)
    {
        LOG_ERROR("Skill Manager: Could not find " << skillReferenceFile << "!");
        free(data);
        return;
    }

    xmlDocPtr doc = xmlParseMemory(data, size);
    free(data);

    if (!doc)
    {
        LOG_ERROR("Skill Manager: Error while parsing skill database ("
                  << absPathFile << ")!");
        return;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node || !xmlStrEqual(node->name, BAD_CAST "skills"))
    {
        LOG_ERROR("Skill Manager: " << absPathFile
                  << " is not a valid database file!");
        xmlFreeDoc(doc);
        return;
    }

    LOG_INFO("Loading skill reference: " << absPathFile);

    for_each_xml_child_node(setnode, node)
    {
        if (xmlStrEqual(setnode->name, BAD_CAST "set"))
        // we don't care about sets server-sided (yet?)
        for_each_xml_child_node(skillnode, setnode)
        {
            if (xmlStrEqual(skillnode->name, BAD_CAST "skill"))
            {
                std::string name = XML::getProperty(skillnode, "name",
                                                    std::string());
                name = utils::toUpper(name);
                int id = XML::getProperty(skillnode, "id", 0);
                if (id && !name.empty())
                {
                    bool duplicateKey = false;
                    for (SkillMap::iterator i = skillMap.begin();
                         i != skillMap.end(); i++)
                    {
                        if (id == i->second)
                        {
                            LOG_ERROR("SkillManager: The same id: " << id
                            << " is given for skill names: " << i->first
                            << " and " << name);
                            LOG_ERROR("The skill reference: " << "'" << name
                            << "': " << id << " will be ignored.");

                            duplicateKey = true;
                            break;
                        }
                    }

                    if (!duplicateKey)
                    {
                        if (XML::getBoolProperty(skillnode, "default", false))
                        {
                            if (!defaultSkillKey.empty())
                            {
                                LOG_WARN("SkillManager: "
                                "Default Skill Key already defined as "
                                << defaultSkillKey
                                << ". Redefinit it as: " << name);
                            }
                            else
                            {
                                LOG_INFO("SkillManager: Defining " << name
                                << " as default weapon-type key.");
                            }
                            defaultSkillKey = name;
                        }
                        skillMap[name] = id;
                    }
                }
            }
        }
    }

    if (::utils::Logger::mVerbosity >= ::utils::Logger::Debug)
    {
        LOG_DEBUG("Skill map in " << skillReferenceFile << ":"
                  << std::endl << "-----");
        for (SkillMap::iterator i = skillMap.begin(); i != skillMap.end(); i++)
        {
            if (!defaultSkillKey.compare(i->first))
            {
                LOG_DEBUG("'" << i->first << "': " << i->second
                          << " (Default)");
            }
            else
            {
                LOG_DEBUG("'" << i->first << "': " << i->second);
            }
        }
        LOG_DEBUG("-----");
    }

    if (defaultSkillKey.empty())
        LOG_WARN("SkillManager: No default weapon-type id was given during "
                 "Skill map loading. Defaults will fall back to id 0.");

    LOG_INFO("Loaded " << skillMap.size() << " skill references from "
             << absPathFile);
}

int SkillManager::getIdFromString(const std::string &name)
{
    // Check if the name is an integer value.
    if (utils::isNumeric(name))
    {
        int val = 0;
        val = utils::stringToInt(name);
        if (val)
        {
            for (SkillMap::iterator i = skillMap.begin(); i != skillMap.end(); i++)
            {
                if (i->second == val)
                    return val;
            }
            LOG_WARN("SkillManager::getIdFromString(): Numeric weapon-type id "
            << val << " not found into " << skillReferenceFile);

            SkillMap::iterator i = skillMap.find(defaultSkillKey);
            if (i != skillMap.end())
            {
                LOG_WARN("Id defaulted to " << defaultSkillKey << ": "
                << i->second);
                return i->second;
            }
            else
            {
                LOG_WARN("Id defaulted to 0.");
                return 0;
            }
        }
        else
        {
            LOG_WARN("SkillManager: Invalid skill id " << name);
            return 0;
        }
    }

    // Convert to upper case for easier finding
    SkillMap::iterator i = skillMap.find(utils::toUpper(name));
    if (i == skillMap.end())
    {
        LOG_WARN("SkillManager: No weapon-type name corresponding to "
                 << utils::toUpper(name) << " into " << skillReferenceFile);
        return 0;
    }
    else
    {
        return i->second;
    }
}
