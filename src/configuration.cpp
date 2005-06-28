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


#include "configuration.h"
//#include "log.h"
//#include "main.h"

#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>
#include "utils/logger.h"

// MSVC libxml2 at the moment doesn't work right when using MinGW, missing this
// function at link time.
#ifdef WIN32
#undef xmlFree
#define xmlFree(x) ;
#endif

ConfigListener::~ConfigListener()
{
}

void Configuration::init(const std::string &filename)
{
    configPath = filename;

    // Do not attempt to read config from non-existant file
    FILE *testFile = fopen(configPath.c_str(), "r");
    if (!testFile) {
        return;
    }
    else {
        fclose(testFile);
    }

    xmlDocPtr doc = xmlReadFile(filename.c_str(), NULL, 0);

    if (!doc) return;

    xmlNodePtr node = xmlDocGetRootElement(doc);

    if (!node || !xmlStrEqual(node->name, BAD_CAST "configuration")) {
	LOG_WARN("Warning: No configuration file (" << filename.c_str() << ")")
        return;
    }

    for (node = node->xmlChildrenNode; node != NULL; node = node->next)
    {
        if (xmlStrEqual(node->name, BAD_CAST "option"))
        {
            xmlChar *name = xmlGetProp(node, BAD_CAST "name");
            xmlChar *value = xmlGetProp(node, BAD_CAST "value");

            if (name && value) {
                options[std::string((const char*)name)] =
                    std::string((const char*)value);
            }

            if (name) xmlFree(name);
            if (value) xmlFree(value);
        }
    }

    xmlFreeDoc(doc);
}

void Configuration::write()
{
    // Do not attempt to write to file that cannot be opened for writing
    FILE *testFile = fopen(configPath.c_str(), "w");
    if (!testFile) {
        return;
    }
    else {
        fclose(testFile);
    }

    xmlTextWriterPtr writer = xmlNewTextWriterFilename(configPath.c_str(), 0);

    if (writer)
    {
        xmlTextWriterSetIndent(writer, 1);
        xmlTextWriterStartDocument(writer, NULL, NULL, NULL);
        xmlTextWriterStartElement(writer, BAD_CAST "configuration");

        std::map<std::string, std::string>::iterator iter;

        for (iter = options.begin(); iter != options.end(); iter++)
        {
	    //logger->log("Configuration::write(%s, \"%s\")",
	    //iter->first.c_str(), iter->second.c_str());

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

void Configuration::setValue(const std::string &key, std::string value)
{
#ifdef __DEBUG
    std::cout << "Configuration::setValue(" << key << ", " << value << ")\n";
#endif
    options[key] = value;

    // Notify listeners
    std::map<std::string, std::list<ConfigListener*> >::iterator list =
        listeners.find(key);

    if (list != listeners.end()) {
        std::list<ConfigListener*>::iterator listener = (*list).second.begin();

        while (listener != (*list).second.end())
        {
            (*listener)->optionChanged(key);
            listener++;
        }
    }
}

void Configuration::setValue(const std::string &key, float value)
{
    std::stringstream ss;
    if (value == floor(value)) {
        ss << (int)value;
    } else {
        ss << value;
    }
    setValue(key, ss.str());
}

std::string Configuration::getValue(const std::string &key, std::string deflt)
{
    std::map<std::string, std::string>::iterator iter = options.find(key);
    if (iter != options.end()) {
        return (*iter).second;
    }
    return deflt;
}

float Configuration::getValue(const std::string &key, float deflt)
{
    std::map<std::string, std::string>::iterator iter = options.find(key);
    if (iter != options.end()) {
        return atof((*iter).second.c_str());
    }
    return deflt;
}

void Configuration::addListener(
        const std::string &key, ConfigListener *listener)
{
    listeners[key].push_front(listener);
}

void Configuration::removeListener(
        const std::string &key, ConfigListener *listener)
{
    std::list<ConfigListener*>::iterator i = listeners[key].begin();

    while (i != listeners[key].end())
    {
        if ((*i) == listener) {
            listeners[key].erase(i);
            return;
        }
        i++;
    }
}
