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


#ifndef _TMW_SQLITE_DATA_PROVIDER_H_
#define _TMW_SQLITE_DATA_PROVIDER_H_


#include <string>

#include "sqlite3.h"

#include "dataprovider.h"


namespace tmw
{
namespace dal
{


/**
 * A SQLite Data Provider.
 */
class SqLiteDataProvider: public DataProvider
{
    public:
        /**
         * Constructor.
         */
        SqLiteDataProvider(void)
            throw();


        /**
         * Destructor.
         */
        ~SqLiteDataProvider(void)
            throw();


        /**
         * Get the database backend name.
         *
         * @return the database backend name.
         */
        DbBackends
        getDbBackend(void) const
            throw();


        /**
         * Create a new database.
         *
         * @param dbName the database name.
         * @param dbPath the database file path (optional)
         *
         * @exception DbCreationFailure if unsuccessful creation.
         * @exception std::exception if unexpected exception.
         */
        void
        createDb(const std::string& dbName,
                 const std::string& dbPath = "")
            throw(DbCreationFailure,
                  std::exception);


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
        void
        connect(const std::string& dbName,
                const std::string& userName,
                const std::string& password)
            throw(DbConnectionFailure,
                  std::exception);


        /**
         * Create a connection to the database.
         *
         * @param dbName the database name.
         * @param dbPath the database file path.
         * @param userName the user name.
         * @param password the user password.
         *
         * @exception DbConnectionFailure if unsuccessful connection.
         * @exception std::exception if unexpected exception.
         */
        void
        connect(const std::string& dbName,
                const std::string& dbPath,
                const std::string& userName,
                const std::string& password)
            throw(DbConnectionFailure,
                  std::exception);


        /**
         * Execute a SQL query.
         *
         * @param sql the SQL query.
         * @param refresh if true, refresh the cache (optional)
         *
         * @return a recordset.
         *
         * @exception DbSqlQueryExecFailure if unsuccessful execution.
         * @exception std::exception if unexpected exception.
         */
        const RecordSet&
        execSql(const std::string& sql,
                const bool refresh = false)
            throw(DbSqlQueryExecFailure,
                  std::exception);


        /**
         * Close the connection to the database.
         *
         * @exception DbDisconnectionFailure if unsuccessful disconnection.
         * @exception std::exception if unexpected exception.
         */
        void
        disconnect(void)
            throw(DbDisconnectionFailure,
                  std::exception);


    private:
        /**
         * The database.
         */
        sqlite3* mDb;
};


} // namespace dal
} // namespace tmw


#endif // _TMW_SQLITE_DATA_PROVIDER_H_
