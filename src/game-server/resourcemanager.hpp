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
 *
 *  $Id$
 */

#ifndef _TMW_RESOURCE_MANAGER_H
#define _TMW_RESOURCE_MANAGER_H

#include <string>

namespace ResourceManager
{
    /**
     * Searches for zip files and adds them to PhysFS search path.
     */
    void initialize();

    /**
     * Checks whether the given file or directory exists in the search path
     */
    bool exists(std::string const &path);

    /**
     * Allocates data into a buffer pointer for raw data loading. The
     * returned data is expected to be freed using <code>free()</code>.
     *
     * @param fileName The name of the file to be loaded.
     * @param fileSize The size of the file that was loaded.
     *
     * @return An allocated byte array containing the data that was loaded,
     *         or <code>NULL</code> on failure.
     * @note The array contains an extra \0 character at position fileSize.
     */
    char *loadFile(std::string const &fileName, int &fileSize);
}

#endif
