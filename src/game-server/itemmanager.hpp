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

#ifndef ITEMMANAGER_H
#define ITEMMANAGER_H

#include <string>

class ItemClass;

namespace ItemManager
{
    /**
     * Loads item reference file.
     */
    void initialize(const std::string &);

    /**
     * Reloads item reference file.
     */
    void reload();

    /**
     * Destroy item classes.
     */
    void deinitialize();

    /**
     * Gets the ItemClass having the given ID.
     */
    ItemClass *getItem(int itemId);

    /**
     * Gets the version of the loaded item database.
     */
    unsigned getDatabaseVersion();
}

#endif
