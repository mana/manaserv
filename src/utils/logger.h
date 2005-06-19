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


#ifndef _TMWSERV_LOGGER_H_
#define _TMWSERV_LOGGER_H_


#include <fstream>
#include <sstream>
#include <string>

#include "singleton.h"


namespace tmwserv
{
namespace utils
{


/**
 * A very simple logger that writes messages to a log file.
 * If the log file is not set, the messages are routed to the standard output
 * or the standard error depending on the level of the message.
 * By default, the messages will be timestamped but the logger can be
 * configured to not prefix the messages with a timestamp.
 *
 * Limitations:
 *     - not thread-safe.
 *
 * Notes:
 *     - this class implements the Meyer's singleton design pattern.
 *
 * Example of use:
 *
 * <code>
 * #include "logger.h"
 *
 * int main(void)
 * {
 *     using namespace tmwserv::utils;
 *
 *     Logger& logger = Logger::instance();
 *     logger.setLogFile("/path/to/logfile");
 *
 *     // log messages using helper macros.
 *     LOG("hello world")
 *     LOG_DEBUG("level: " << 3)
 *     LOG_INFO("init sound")
 *     LOG_WARN("not implemented")
 *     LOG_ERROR("resource not found")
 *     LOG_FATAL("unable to init graphics")
 *
 *     // log messages using APIs.
 *     logger.log("hello world");
 *
 *     std::ostringstream os;
 *     os << "level: " << 3;
 *     logger.debug(os.str());
 *
 *     logger.info("init sound");
 *     logger.warn("not implemented");
 *     logger.error("resource not found");
 *     logger.fatal("unable to init graphics");
 *
 *     return 0;
 * }
 * </code>
 */
class Logger: public Singleton<Logger>
{
    // friend so that Singleton can call the constructor.
    friend class Singleton<Logger>;


    public:
        /**
         * Set the log file.
         *
         * This method will open the log file for writing, the former file
         * contents are removed.
         *
         * @param logFile the log file name (may include path).
         *
         * @exception std::ios::failure if the log file could not be opened.
         */
        void
        setLogFile(const std::string& logFile);


        /**
         * Add/remove the timestamp.
         *
         * @param flag if true, a log message will always be timestamped
         *             (default = true).
         */
        void
        setTimestamp(bool flag = true)
            throw();


        /**
         * Set tee mode.
         *
         * @param flag if true, write messages to both the standard (or error)
         *        output and the log file (if set) (default = true).
         */
        void
        setTeeMode(bool flag = true)
            throw();


        /**
         * Log a generic message.
         *
         * @param msg the message to log.
         *
         * @exception std::ios::failure.
         */
        void
        log(const std::string& msg);


        /**
         * Log a debug message.
         *
         * @param msg the message to log.
         *
         * @exception std::ios::failure.
         */
        void
        debug(const std::string& msg);


        /**
         * Log an info message.
         *
         * @param msg the message to log.
         *
         * @exception std::ios::failure.
         */
        void
        info(const std::string& msg);


        /**
         * Log a warn message.
         *
         * @param msg the message to log.
         *
         * @exception std::ios::failure.
         */
        void
        warn(const std::string& msg);


        /**
         * Log an error message.
         *
         * @param msg the message to log.
         *
         * @exception std::ios::failure.
         */
        void
        error(const std::string& msg);


        /**
         * Log a fatal error message.
         *
         * @param msg the message to log.
         *
         * @exception std::ios::failure.
         */
        void
        fatal(const std::string& msg);


    private:
        /**
         * Default constructor.
         */
        Logger(void)
            throw();


        /**
         * Destructor.
         */
        ~Logger(void)
            throw();


        /**
         * Copy constructor.
         */
        Logger(const Logger& rhs);


        /**
         * Assignment operator.
         */
        Logger&
        operator=(const Logger& rhs);


        /**
         * Log a generic message.
         *
         * @param os the output stream.
         * @param msg the message to log.
         * @param prefix the message prefix (default = "").
         *
         * @exception std::ios::failure.
         */
        void
        log(std::ostream& os,
            const std::string& msg,
            const std::string& prefix = "");


        /**
         * Get the current time.
         *
         * @return the current time as string.
         */
        std::string
        getCurrentTime(void);


    private:
        std::ofstream mLogFile; /**< the log file */
        bool mHasTimestamp;     /**< the timestamp flag */
        bool mTeeMode;          /**< the tee mode flag */
};


} // namespace utils
} // namespace tmwserv


// HELPER MACROS


#define LOG(msg)                                                \
    {                                                           \
        std::ostringstream os;                                  \
        os << msg;                                              \
        ::tmwserv::utils::Logger::instance().log(os.str());     \
    }


#define LOG_DEBUG(msg)                                          \
    {                                                           \
        std::ostringstream os;                                  \
        os << msg;                                              \
        ::tmwserv::utils::Logger::instance().debug(os.str());   \
    }


#define LOG_INFO(msg)                                           \
    {                                                           \
        std::ostringstream os;                                  \
        os << msg;                                              \
        ::tmwserv::utils::Logger::instance().info(os.str());    \
    }


#define LOG_WARN(msg)                                           \
    {                                                           \
        std::ostringstream os;                                  \
        os << msg;                                              \
        ::tmwserv::utils::Logger::instance().warn(os.str());    \
    }


#define LOG_ERROR(msg)                                          \
    {                                                           \
        std::ostringstream os;                                  \
        os << msg;                                              \
        ::tmwserv::utils::Logger::instance().error(os.str());   \
    }


#define LOG_FATAL(msg)                                          \
    {                                                           \
        std::ostringstream os;                                  \
        os << msg;                                              \
        ::tmwserv::utils::Logger::instance().fatal(os.str());   \
    }


#endif // _TMWSERV_LOGGER_H_
