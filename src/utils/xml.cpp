/*
 *  The Mana World
 *  Copyright 2006 The Mana World Development Team
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

#include <cstdlib>

#include "utils/xml.hpp"

namespace XML
{

int getProperty(xmlNodePtr node, const char *name, int def)
{
    if (xmlChar *prop = xmlGetProp(node, BAD_CAST name))
    {
        int ret = atoi((char*)prop);
        xmlFree(prop);
        return ret;
    }
    return def;
}

double getFloatProperty(xmlNodePtr node, const char* name, double def)
{
    double &ret = def;

    xmlChar *prop = xmlGetProp(node, BAD_CAST name);
    if (prop) {
        ret = atof((char*)prop);
        xmlFree(prop);
    }

    return ret;
}

std::string getProperty(xmlNodePtr node, const char *name,
                        const std::string &def)
{
    if (xmlChar *prop = xmlGetProp(node, BAD_CAST name))
    {
        std::string val = (char *)prop;
        xmlFree(prop);
        return val;
    }
    return def;
}

} // namespace XML
