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

#include "resourcemanager.h"

#include <physfs.h>

#include "utils/logger.h"

#ifdef WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#define TMWSERV_DATADIR ""

ResourceManager *ResourceManager::instance = NULL;

ResourceManager::ResourceManager()
{
    // Add zip files to PhysicsFS
    searchAndAddZipFiles();
}

ResourceManager::~ResourceManager()
{
}

ResourceManager*
ResourceManager::getInstance()
{
    // Create a new instance if necessary.
    if (instance == NULL) instance = new ResourceManager();
    return instance;
}

void
ResourceManager::deleteInstance()
{
    if (instance != NULL) {
        delete instance;
        instance = NULL;
    }
}

void
ResourceManager::searchAndAddZipFiles()
{
    PHYSFS_permitSymbolicLinks(1);
    // Add the main data directory to our PhysicsFS search path
    PHYSFS_addToSearchPath("data", 1);
    PHYSFS_addToSearchPath(TMWSERV_DATADIR "data", 1);

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
    getcwd(programPath, 256);
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

void*
ResourceManager::loadFile(const std::string &fileName, int &fileSize)
{
    // If the file doesn't exist indicate failure
    if (!PHYSFS_exists(fileName.c_str())) {
        LOG_WARN("Warning: " << fileName << " not found!");
        return NULL;
    }

    // Attempt to open the specified file using PhysicsFS
    PHYSFS_file* file = PHYSFS_openRead(fileName.c_str());

    // If the handler is an invalid pointer indicate failure
    if (file == NULL) {
        LOG_WARN("Warning: " << fileName << " failed to load!");
        return NULL;
    }

    // Get the size of the file
    fileSize = PHYSFS_fileLength(file);

    // Allocate memory and load the file
    void *buffer = malloc(fileSize);
    PHYSFS_read(file, buffer, 1, fileSize);

    // Close the file and let the user deallocate the memory
    PHYSFS_close(file);

    return buffer;
}

std::vector<std::string>
ResourceManager::loadTextFile(const std::string &fileName)
{
    int contentsLength;
    char *fileContents = (char*)loadFile(fileName, contentsLength);
    std::vector<std::string> lines;

    if (!fileContents)
    {
        LOG_ERROR("Couldn't load text file: " << fileName);
        return lines;
    }

    // Reallocate and include terminating 0 character
    fileContents = (char*)realloc(fileContents, contentsLength + 1);
    fileContents[contentsLength] = '\0';

    // Tokenize and add each line separately
    char *line = strtok(fileContents, "\n");
    while (line != NULL)
    {
        lines.push_back(line);
        line = strtok(NULL, "\n");
    }

    free(fileContents);
    return lines;
}
