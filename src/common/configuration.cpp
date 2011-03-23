/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
 *  Copyright (C) 2010  The Mana Developers
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

#include <cmath>
#include <map>
#include <libxml/xmlreader.h>

#include "common/configuration.h"

#include "utils/logger.h"
#include "utils/xml.h"
#include "utils/string.h"

/**< Persistent configuration. */
static std::map< std::string, std::string > options;
/**< Location of config file. */
static std::string configPath;

bool Configuration::initialize(const std::string &filename)
{
    configPath = filename;

    XML::Document doc(filename, false);
    xmlNodePtr node = doc.rootNode();

    if (!node || !xmlStrEqual(node->name, BAD_CAST "configuration")) {
        LOG_WARN("No configuration file '" << filename.c_str() << "'.");
        return false;
    }

    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
        if (!xmlStrEqual(node->name, BAD_CAST "option"))
            continue;
        if (!XML::hasProperty(node, "name") || !XML::hasProperty(node, "value"))
            continue;

        std::string key = XML::getProperty(node, "name", std::string());
        std::string value = XML::getProperty(node, "value", std::string());

        if (!key.empty())
            options[key] = value;
    }

    return true;
}

void Configuration::deinitialize()
{
}

std::string Configuration::getValue(const std::string &key,
                                    const std::string &deflt)
{
    std::map<std::string, std::string>::iterator iter = options.find(key);
    if (iter == options.end())
        return deflt;
    return iter->second;
}

int Configuration::getValue(const std::string &key, int deflt)
{
    std::map<std::string, std::string>::iterator iter = options.find(key);
    if (iter == options.end())
        return deflt;
    return atoi(iter->second.c_str());
}

bool Configuration::getBoolValue(const std::string &key, bool deflt)
{
    std::map<std::string, std::string>::iterator iter = options.find(key);
    if (iter == options.end())
        return deflt;
    return utils::stringToBool(iter->second.c_str(), deflt);
}
