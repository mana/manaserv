/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
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

#ifndef XML_H
#define XML_H

#include <string>
#include <libxml/tree.h>

#include "game-server/item.hpp"

namespace XML
{
    /**
     * Gets an integer property from an xmlNodePtr.
     */
    int getProperty(xmlNodePtr node, const char *name, int def);

    /**
     * Gets a string property from an xmlNodePtr.
     */
    std::string getProperty(xmlNodePtr node, const char *name,
                            const std::string &def);

    /**
     * Gets an floating point property from an xmlNodePtr.
     */
    double getFloatProperty(xmlNodePtr node, const char *name, double def);
}

#define for_each_xml_child_node(var, parent) \
    for (xmlNodePtr var = parent->xmlChildrenNode; var; var = var->next)

#endif
