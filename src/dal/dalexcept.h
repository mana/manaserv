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


#ifndef _TMWSERV_DAL_EXCEPT_H_
#define _TMWSERV_DAL_EXCEPT_H_


#include <string>

namespace dal
{


/**
 * Default database exception.
 */
class DbException: public std::exception
{
    public:
        /**
         * Constructor.
         *
         * @param msg the error message.
         */
        DbException(const std::string& msg)
            throw()
                : mMsg(msg)
        {
            // NOOP
        }


        /**
         * Destructor.
         */
        ~DbException(void)
            throw()
        {
            // NOOP
        }


        /**
         * Get the error message.
         *
         * @return the error message.
         */
        virtual const char*
        what(void) const
            throw()
        {
            return mMsg.c_str();
        }


    private:
        std::string mMsg;
};


/**
 * Database connection failure.
 */
class DbConnectionFailure: public DbException
{
    public:
        /**
         * Default constructor.
         */
        DbConnectionFailure(void)
            throw()
                : DbException("")
        {
            // NOOP
        }


        /**
         * Constructor.
         *
         * @param msg the error message.
         */
        DbConnectionFailure(const std::string& msg)
            throw()
                : DbException(msg)
        {
            // NOOP
        }
};


/**
 * Database disconnection failure.
 */
class DbDisconnectionFailure: public DbException
{
    public:
        /**
         * Default constructor.
         */
        DbDisconnectionFailure(void)
            throw()
                : DbException("")
        {
            // NOOP
        }


        /**
         * Constructor.
         *
         * @param msg the error message.
         */
        DbDisconnectionFailure(const std::string& msg)
            throw()
                : DbException(msg)
        {
            // NOOP
        }
};


/**
 * SQL query execution failure.
 */
class DbSqlQueryExecFailure: public DbException
{
    public:
        /**
         * Default constructor.
         */
        DbSqlQueryExecFailure(void)
            throw()
                : DbException("")
        {
            // NOOP
        }


        /**
         * Constructor.
         *
         * @param msg the error message.
         */
        DbSqlQueryExecFailure(const std::string& msg)
            throw()
                : DbException(msg)
        {
            // NOOP
        }
};


/**
 * Already set exception.
 */
class AlreadySetException: public std::exception
{
};


/**
 * Missing column headers exception.
 */
class RsColumnHeadersNotSet: public std::exception
{
};


} // namespace dal

#endif // _TMWSERV_DAL_EXCEPT_H_
