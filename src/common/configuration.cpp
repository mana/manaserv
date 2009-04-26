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
 */

#include <cmath>
#include <map>
#include <libxml/xmlwriter.h>

#include "common/configuration.hpp"

#include "utils/logger.h"
#include "utils/xml.hpp"

/**< Persistent configuration. */
static std::map< std::string, std::string > options;
/**< Location of config file. */
static std::string configPath;

void Configuration::initialize(const std::string &filename)
{
    configPath = filename;

    xmlDocPtr doc = xmlReadFile(filename.c_str(), NULL, 0);

    if (!doc) return;

    xmlNodePtr node = xmlDocGetRootElement(doc);

    if (!node || !xmlStrEqual(node->name, BAD_CAST "configuration")) {
	LOG_WARN("No configuration file '" << filename.c_str() << "'.");
        return;
    }

    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
        if (xmlStrEqual(node->name, BAD_CAST "option"))
        {
            std::string key = XML::getProperty(node, "name", "");
            std::string value = XML::getProperty(node, "value", "");

            if (!key.empty() && !value.empty())
            {
                options[key] = value;
            }
        }
    }

    xmlFreeDoc(doc);
}

void Configuration::deinitialize()
{
    xmlTextWriterPtr writer = xmlNewTextWriterFilename(configPath.c_str(), 0);

    if (writer)
    {
        xmlTextWriterSetIndent(writer, 1);
        xmlTextWriterStartDocument(writer, NULL, NULL, NULL);
        xmlTextWriterStartElement(writer, BAD_CAST "configuration");

        std::map<std::string, std::string>::iterator iter;

        for (iter = options.begin(); iter != options.end(); iter++)
        {
            xmlTextWriterStartElement(writer, BAD_CAST "option");
            xmlTextWriterWriteAttribute(writer,
                    BAD_CAST "name", BAD_CAST iter->first.c_str());
            xmlTextWriterWriteAttribute(writer,
                    BAD_CAST "value", BAD_CAST iter->second.c_str());
            xmlTextWriterEndElement(writer);
        }

        xmlTextWriterEndDocument(writer);
        xmlFreeTextWriter(writer);
    }
}

void Configuration::setValue(const std::string &key, const std::string &value)
{
    options[key] = value;
}

void Configuration::setValue(const std::string &key, int value)
{
    std::ostringstream ss;
    ss << value;
    setValue(key, ss.str());
}

const std::string &Configuration::getValue(const std::string &key,
                                           const std::string &deflt)
{
    std::map<std::string, std::string>::iterator iter = options.find(key);
    if (iter == options.end()) return deflt;
    return iter->second;
}

int Configuration::getValue(const std::string &key, int deflt)
{
    std::map<std::string, std::string>::iterator iter = options.find(key);
    if (iter == options.end()) return deflt;
    return atoi(iter->second.c_str());
}
