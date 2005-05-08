/*
 *  The Mana World Server
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

#include "log.h"
#ifdef WIN32
#include <windows.h>
#endif

#include <sstream>


Logger::Logger(const std::string &logFilename)
{
    logFile.open(logFilename.c_str(), std::ios_base::trunc);

    if (!logFile.is_open())
    {
        std::cout << "Warning: error while opening " << logFilename <<
            " for writing.\n";
    }
}

Logger::~Logger()
{
    if (logFile.is_open())
    {
        logFile.close();
    }
}

void Logger::log(const char *log_text, ...)
{
    if (logFile.is_open())
    {
        char* buf = new char[1024];
        va_list ap;
        time_t t;

        // Use a temporary buffer to fill in the variables
        va_start(ap, log_text);
        vsprintf(buf, log_text, ap);
        va_end(ap);

        // Get the current system time
        time(&t);

        // Print the log entry
        std::stringstream timeStr;
        timeStr << "[";
        timeStr << ((((t / 60) / 60) % 24 < 10) ? "0" : "");
        timeStr << (int)(((t / 60) / 60) % 24);
        timeStr << ":";
        timeStr << (((t / 60) % 60 < 10) ? "0" : "");
        timeStr << (int)((t / 60) % 60);
        timeStr << ":";
        timeStr << ((t % 60 < 10) ? "0" : "");
        timeStr << (int)(t % 60);
        timeStr << "] ";
        
        logFile << timeStr.str() << buf << std::endl;

        // Delete temporary buffer
        delete[] buf;
    }
}

void Logger::error(const std::string &error_text)
{
    log("Error: %s", error_text.c_str());
#ifdef WIN32
    MessageBox(NULL, error_text.c_str(), "Error", MB_ICONERROR | MB_OK);
#else
    std::cerr << "Error: " << error_text << std::endl;
#endif
    exit(1);
}

/**
 * And the instance used in the server
 */
Logger *logger;
