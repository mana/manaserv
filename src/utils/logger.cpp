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

#include "logger.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>

#ifdef WIN32
#include <windows.h>
#endif

namespace utils
{

static std::ofstream mLogFile;     /**< Log file. */
bool Logger::mHasTimestamp = true; /**< Timestamp flag. */
bool Logger::mTeeMode = false;     /**< Tee mode flag. */
Logger::Level Logger::mVerbosity = Logger::INFO; /**< Verbosity level. */

/**
  * Gets the current time.
  *
  * @return the current time as string.
  */
static std::string getCurrentTime()
{
    time_t now;
    tm local;

    // get current time_t value
    time(&now);

    // convert time_t to tm struct to break the time into individual
    // constituents
    local = *(localtime(&now));

    // stringify the time, the format is: [hh:mm:ss]
    using namespace std;
    ostringstream os;
    os << "[" << setw(2) << setfill('0') << local.tm_hour
       << ":" << setw(2) << setfill('0') << local.tm_min
       << ":" << setw(2) << setfill('0') << local.tm_sec
       << "]";

    return os.str();
}

void Logger::output(std::ostream &os, const std::string &msg, const char *prefix)
{
    if (mHasTimestamp)
    {
        os << getCurrentTime() << ' ';
    }

    if (prefix)
    {
        os << prefix << ' ';
    }

    os << msg << std::endl;
}

void Logger::setLogFile(const std::string &logFile)
{
    // Close the current log file.
    if (mLogFile.is_open())
    {
        mLogFile.close();
    }

    // Open the file for output and remove the former file contents.
    mLogFile.open(logFile.c_str(), std::ios::trunc);

    if (!mLogFile.is_open())
    {
        throw std::ios::failure("unable to open " + logFile + "for writing");
    }
    else
    {
        // by default the streams do not throw any exception
        // let std::ios::failbit and std::ios::badbit throw exceptions.
        mLogFile.exceptions(std::ios::failbit | std::ios::badbit);
    }
}

void Logger::output(const std::string &msg, Level atVerbosity)
{
    static const char *prefixes[] =
    {
        "[FTL]",
        "[ERR]",
        "[WRN]",
        "[INF]",
        "[DBG]"
    };

    if (mVerbosity >= atVerbosity)
    {
        bool open = mLogFile.is_open();

        if (open)
        {
            output(mLogFile, msg, prefixes[atVerbosity]);
        }

        if (!open || mTeeMode)
        {
            output(atVerbosity <= WARN ? std::cerr : std::cout,
                   msg, prefixes[atVerbosity]);
        }
    }
}

} // namespace utils
