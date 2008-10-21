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

#ifndef _TMWSERV_SQLITE_DATA_PROVIDER_H_
#define _TMWSERV_SQLITE_DATA_PROVIDER_H_

#include <iosfwd>
#include "limits.h"
#include <sqlite3.h>
#include "common/configuration.hpp"


// sqlite3_int64 is the preferred new datatype for 64-bit int values.
// see: http://www.sqlite.org/capi3ref.html#sqlite3_int64
#ifndef sqlite3_int64
typedef sqlite_int64 sqlite3_int64;
#endif


#include "dataprovider.h"

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
         * Get the name of the database backend.
         *
         * @return the database backend name.
         */
        DbBackends
        getDbBackend(void) const
            throw();


        /**
         * Create a connection to the database.
         *
         * @exception DbConnectionFailure if unsuccessful connection.
         */
        void connect();


        /**
         * Execute a SQL query.
         *
         * @param sql the SQL query.
         * @param refresh if true, refresh the cache (default = false).
         *
         * @return a recordset.
         *
         * @exception DbSqlQueryExecFailure if unsuccessful execution.
         * @exception std::runtime_error if trying to query a closed database.
         */
        const RecordSet&
        execSql(const std::string& sql,
                const bool refresh = false);


        /**
         * Close the connection to the database.
         *
         * @exception DbDisconnectionFailure if unsuccessful disconnection.
         */
        void
        disconnect(void);

        /**
         * Starts a transaction.
         *
         * @exception std::runtime_error if a transaction is still open
         */
        void
        beginTransaction(void)
            throw (std::runtime_error);

        /**
         * Commits a transaction.
         *
         * @exception std::runtime_error if no connection is currently open.
         */
        void
        commitTransaction(void)
            throw (std::runtime_error);

        /**
         * Rollback a transaction.
         *
         * @exception std::runtime_error if no connection is currently open.
         */
        void
        rollbackTransaction(void)
            throw (std::runtime_error);

        /**
         * Returns the number of changed rows by the last executed SQL
         * statement.
         *
         * @return Number of rows that have changed.
         */
        const unsigned int
        getModifiedRows(void) const;

        /**
         * Returns the last inserted value of an autoincrement column after an
         * INSERT statement.
         *
         * @return last autoincrement value.
         */
        const unsigned int
        getLastId(void) const;

    private:

        /** defines the name of the database config parameter */
        static const std::string CFGPARAM_SQLITE_DB;
        /** defines the default value of the CFGPARAM_SQLITE_DB parameter */
        static const std::string CFGPARAM_SQLITE_DB_DEF;

        /**
         * Returns wheter the connection has a open transaction or is in auto-
         * commit mode.
         *
         * @return true, if a transaction is open.
         */
        const bool
        inTransaction(void) const;

        sqlite3* mDb; /**< the handle to the database connection */
};


} // namespace dal

#endif // _TMWSERV_SQLITE_DATA_PROVIDER_H_
