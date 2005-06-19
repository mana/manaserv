/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */


#include <ctime>
#include <iomanip>
#include <iostream>

#ifdef WIN32
#include <windows.h>
#endif

#include "logger.h"


namespace tmwserv
{
namespace utils
{


/**
 * Default constructor.
 */
Logger::Logger(void)
    throw()
{
    // NOOP
}


/**
 * Destructor.
 */
Logger::~Logger(void)
    throw()
{
    // the destructor of std::ofstream takes care of closing the file
    // if it is still open :)
}


/**
 * Set the log file.
 */
void
Logger::setLogFile(const std::string& logFile)
{
    // close the current log file.
    if (mLogFile.is_open()) {
        mLogFile.close();
    }

    // open the file for output and remove the former file contents.
    mLogFile.open(logFile.c_str(), std::ios::trunc);

    if (!mLogFile.is_open()) {
        std::string msg("unable to open ");
        msg += logFile;
        msg += " for writing.";

        throw std::ios::failure(msg);
    }
    else {
        // by default the streams do not throw any exception
        // let std::ios::failbit and std::ios::badbit throw exceptions.
        mLogFile.exceptions(std::ios::failbit | std::ios::badbit);
    }
}


/**
 * Add/remove the timestamp.
 */
void
Logger::setTimestamp(bool flag)
    throw()
{
    mHasTimestamp = flag;
}


/**
 * Set tee mode.
 */
void
Logger::setTeeMode(bool flag)
    throw()
{
    mTeeMode = flag;
}


/**
 * Log a generic message.
 */
void
Logger::log(const std::string& msg)
{
    if (mTeeMode) {
        log(std::cout, msg);

        if (mLogFile.is_open()) {
            log(mLogFile, msg);
        }
    }
    else {
        log((mLogFile.is_open() ? mLogFile : std::cout), msg);
    }
}


/**
 * Log a debug message.
 */
void
Logger::debug(const std::string& msg)
{
    if (mTeeMode) {
        log(std::cout, msg, "[DBG]");

        if (mLogFile.is_open()) {
            log(mLogFile, msg, "[DBG]");
        }
    }
    else {
        log((mLogFile.is_open() ? mLogFile : std::cout), msg, "[DBG]");
    }
}


/**
 * Log an info message.
 */
void
Logger::info(const std::string& msg)
{
    if (mTeeMode) {
        log(std::cout, msg, "[INF]");

        if (mLogFile.is_open()) {
            log(mLogFile, msg, "[INF]");
        }
    }
    else {
        log((mLogFile.is_open() ? mLogFile : std::cout), msg, "[INF]");
    }
}


/**
 * Log a warn message.
 */
void
Logger::warn(const std::string& msg)
{
    if (mTeeMode) {
        log(std::cerr, msg, "[WRN]");

        if (mLogFile.is_open()) {
            log(mLogFile, msg, "[WRN]");
        }
    }
    else {
        log((mLogFile.is_open() ? mLogFile : std::cerr), msg, "[WRN]");
    }
}


/**
 * Log an error message.
 */
void
Logger::error(const std::string& msg)
{
    if (mTeeMode) {
        log(std::cerr, msg, "[ERR]");

        if (mLogFile.is_open()) {
            log(mLogFile, msg, "[ERR]");
        }
    }
    else {
        log((mLogFile.is_open() ? mLogFile : std::cerr), msg, "[ERR]");
    }
}


/**
 * Log a fatal error message.
 */
void
Logger::fatal(const std::string& msg)
{
    if (mTeeMode) {
        log(std::cerr, msg, "[FTL]");

        if (mLogFile.is_open()) {
            log(mLogFile, msg, "[FTL]");
        }
    }
    else {
        log((mLogFile.is_open() ? mLogFile : std::cerr), msg, "[FTL]");
    }

#ifdef WIN32
    MessageBox(NULL, msg.c_str(), "Fatal error", MB_ICONERROR | MB_OK);
#endif
}


/**
 * Log a generic message.
 */
void
Logger::log(std::ostream& os,
            const std::string& msg,
            const std::string& prefix)
{
    if (mHasTimestamp) {
        os << getCurrentTime() << " ";
    }

    if (prefix != "") {
        os << prefix << " ";
    }

    os << msg << std::endl;
}


/**
 * Get the current time.
 */
std::string
Logger::getCurrentTime(void)
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


} // namespace utils
} // namespace tmwserv
