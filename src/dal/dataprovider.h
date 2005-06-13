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


#ifndef _TMW_DATA_PROVIDER_H_
#define _TMW_DATA_PROVIDER_H_


#include <string>

#include "dalexcept.h"
#include "recordset.h"


namespace tmw
{
namespace dal
{


/**
 * Enumeration type for the database backends.
 */
typedef enum {
    MYSQL,
    SQLITE
} DbBackends;


/**
 * An abstract data provider.
 */
class DataProvider
{
    public:
        /**
         * Constructor.
         */
        DataProvider(void)
            throw();


        /**
         * Destructor.
         */
        ~DataProvider(void)
            throw();


        /**
         * Get the database backend name.
         *
         * @return the database backend name.
         */
        virtual DbBackends
        getDbBackend(void) const
            throw() = 0;


        /**
         * Create a new database.
         *
         * @param dbName the database name.
         * @param dbPath the database file path (optional)
         *
         * @exception DbCreationFailure if unsuccessful creation.
         * @exception std::exception if unexpected exception.
         */
        virtual void
        createDb(const std::string& dbName,
                 const std::string& dbPath = "")
            throw(DbCreationFailure,
                  std::exception) = 0;


        /**
         * Create a connection to the database.
         *
         * @param dbName the database name.
         * @param userName the user name.
         * @param password the user password.
         *
         * @exception DbConnectionFailure if unsuccessful connection.
         * @exception std::exception if unexpected exception.
         */
        virtual void
        connect(const std::string& dbName,
                const std::string& userName,
                const std::string& password)
            throw(DbConnectionFailure,
                  std::exception) = 0;


        /**
         * Execute a SQL query.
         *
         * @param sql the SQL query.
         *
         * @return a recordset.
         *
         * @exception DbSqlQueryExecFailure if unsuccessful execution.
         * @exception std::exception if unexpected exception.
         */
        virtual const RecordSet&
        execSql(const std::string& sql)
            throw(DbSqlQueryExecFailure,
                  std::exception) = 0;


        /**
         * Close the connection to the database.
         *
         * @exception DbDisconnectionFailure if unsuccessful disconnection.
         * @exception std::exception if unexpected exception.
         */
        virtual void
        disconnect(void)
            throw(DbDisconnectionFailure,
                  std::exception) = 0;


        /**
         * Get the connection status.
         *
         * @return true if connected.
         */
        bool
        isConnected(void) const
            throw();


    protected:
        /**
         * The connection status.
         */
        bool mIsConnected;


        /**
         * Cache the last SQL query.
         */
        std::string mSql;


        /**
         * Cache the result of the last SQL query.
         */
        RecordSet mRecordSet;
};


} // namespace dal
} // namespace tmw


#endif // _TMW_DATA_PROVIDER_H_
