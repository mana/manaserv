/*
 *  The Mana World
 *  Copyright 2004-2007 The Mana World Development Team
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

#ifndef __INIREAD_H
#define __INIREAD_H

#include <string>

namespace Configuration
{
    /**
     * Loads the configuration options into memory.
     * @param filename path to the configuration file .
     */
    void initialize(const std::string &filename);

    /**
     * Writes the current settings back to the configuration file.
     */
    void deinitialize();

    /**
     * Sets an option to a given value.
     * @param key option identifier.
     * @param value Value.
     */
    void setValue(const std::string &key, const std::string &value);

    /**
     * Sets an option to a given value.
     * @param key option identifier.
     * @param value value.
     */
    void setValue(const std::string &key, int value);

    /**
     * Gets an option as a string.
     * @param key option identifier.
     * @param deflt default value.
     */
    const std::string &getValue(const std::string &key, const std::string &deflt);

    /**
     * Gets an option as a string.
     * @param key option identifier.
     * @param deflt default value.
     */
    int getValue(const std::string &key, int deflt);
}

#ifndef DEFAULT_SERVER_PORT
#define DEFAULT_SERVER_PORT 9601
#endif
#endif
