/*
 *  The Mana Server
 *  Copyright (C) 2004-2007  The Mana World Development Team
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

#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include <physfs.h>

#include "game-server/resourcemanager.hpp"

#include "utils/logger.h"

#define PKG_DATADIR ""

void ResourceManager::initialize()
{
    PHYSFS_permitSymbolicLinks(1);
    // Add the main data directory to our PhysicsFS search path
    PHYSFS_addToSearchPath("data", 1);
    PHYSFS_addToSearchPath(PKG_DATADIR "data", 1);

#ifdef _WIN32
    // Define the path in which to search
    std::string searchString = std::string("data/*.zip");

    // Create our find file data structure
    struct _finddata_t findFileInfo;

    // Find the first zipped file
    long handle =
        static_cast<long>(::_findfirst(searchString.c_str(), &findFileInfo));
    long file = handle;

    // Loop until all files we're searching for are found
    while (file >= 0) {
        // Define the file path string
        std::string filePath = std::string("data/") +
            std::string(findFileInfo.name);

        LOG_INFO("Adding to PhysicsFS: " << findFileInfo.name);

        // Add the zip file to our PhysicsFS search path
        PHYSFS_addToSearchPath(filePath.c_str(), 1);

        // Find the next file
        file = ::_findnext(handle, &findFileInfo);
    }

    // Shutdown findfile stuff
    ::_findclose(handle);
#else
    // Retrieve the current path
    char programPath[256];
    if (!getcwd(programPath, 256))
        strcpy(programPath, ".");
    strncat(programPath, "/data", 256 - strlen(programPath) - 1);

    // Create our directory structure
    DIR *dir = opendir(programPath);

    // Return if the directory is invalid
    if (dir == NULL) {
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(dir)) != NULL)
    {
        char *ext = strstr(direntry->d_name, ".zip");

        if (ext != NULL && strcmp(ext, ".zip") == 0)
        {
            // Define the file path string
            std::string filePath = std::string(programPath) +
                std::string("/") + std::string(direntry->d_name);

            LOG_INFO("Adding to PhysicsFS: " << filePath);

            // Add the zip file to our PhysicsFS search path
            PHYSFS_addToSearchPath(filePath.c_str(), 1);
        }
    }

    closedir(dir);
#endif
}

bool ResourceManager::exists(const std::string &path)
{
    return PHYSFS_exists(path.c_str());
}

char *ResourceManager::loadFile(const std::string &fileName, int &fileSize)
{
    // Attempt to open the specified file using PhysicsFS
    PHYSFS_file* file = PHYSFS_openRead(fileName.c_str());

    // If the handler is an invalid pointer indicate failure
    if (file == NULL)
    {
        LOG_WARN("Failed to load '" << fileName << "': "
                 << PHYSFS_getLastError());
        return NULL;
    }

    // Get the size of the file
    fileSize = PHYSFS_fileLength(file);

    // Allocate memory and load the file
    char *buffer = (char *)malloc(fileSize + 1);
    if (PHYSFS_read(file, buffer, 1, fileSize) != fileSize)
    {
        free(buffer);
        LOG_WARN("Failed to load '" << fileName << "': "
                 << PHYSFS_getLastError());
        return NULL;
    }

    // Close the file and let the user deallocate the memory
    PHYSFS_close(file);

    // Add a trailing nul character, so that the file can be used as a string
    buffer[fileSize] = 0;
    return buffer;
}
