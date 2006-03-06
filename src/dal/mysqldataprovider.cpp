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

#include "mysqldataprovider.h"

#include "dalexcept.h"

namespace tmwserv
{
namespace dal
{


/**
 * Constructor.
 */
MySqlDataProvider::MySqlDataProvider(void)
    throw()
        : mDb(0)
{
    // NOOP
}


/**
 * Destructor.
 */
MySqlDataProvider::~MySqlDataProvider(void)
    throw()
{
    // we are using the MySQL C API, there are no exceptions to catch.

    // make sure that the database is closed.
    // disconnect() calls mysql_close() which takes care of freeing
    // the memory allocated for the handle.
    if (mIsConnected) {
        disconnect();
    }
}


/**
 * Get the database backend name.
 */
DbBackends
MySqlDataProvider::getDbBackend(void) const
    throw()
{
    return DB_BKEND_MYSQL;
}


/**
 * Create a connection to the database.
 */
void
MySqlDataProvider::connect(const std::string& dbName,
                           const std::string& userName,
                           const std::string& password)
{
    if (mIsConnected) {
        return;
    }

    // allocate and initialize a new MySQL object suitable
    // for mysql_real_connect().
    mDb = mysql_init(NULL);

    if (!mDb) {
        throw DbConnectionFailure(
            "unable to initialize the MySQL library: no memory");
    }

    // insert connection options here.

    // actually establish the connection.
    if (!mysql_real_connect(mDb,              // handle to the connection
                            NULL,             // localhost
                            userName.c_str(), // user name
                            password.c_str(), // user password
                            dbName.c_str(),   // database name
                            0,                // use default TCP port
                            NULL,             // use defaut socket
                            0))               // client flags
    {
        std::string msg(mysql_error(mDb));
        mysql_close(mDb);

        throw DbConnectionFailure(msg);
    }

    // Save the Db Name.
    mDbName = dbName;

    mIsConnected = true;
}


/**
 * Execute a SQL query.
 */
const RecordSet&
MySqlDataProvider::execSql(const std::string& sql,
                           const bool refresh)
{
    if (!mIsConnected) {
        throw std::runtime_error("not connected to database");
    }

    // do something only if the query is different from the previous
    // or if the cache must be refreshed
    // otherwise just return the recordset from cache.
    if (refresh || (sql != mSql)) {
        mRecordSet.clear();

        // actually execute the query.
        if (mysql_query(mDb, sql.c_str()) != 0) {
            throw DbSqlQueryExecFailure(mysql_error(mDb));
        }

        if (mysql_field_count(mDb) > 0) {
            MYSQL_RES* res;

            // get the result of the query.
            if (!(res = mysql_store_result(mDb))) {
                throw DbSqlQueryExecFailure(mysql_error(mDb));
            }

            // set the field names.
            unsigned int nFields = mysql_num_fields(res);
            MYSQL_FIELD* fields = mysql_fetch_fields(res);
            Row fieldNames;
            for (unsigned int i = 0; i < nFields; ++i) {
                fieldNames.push_back(fields[i].name);
            }
            mRecordSet.setColumnHeaders(fieldNames);

            // populate the RecordSet.
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                Row r;

                for (unsigned int i = 0; i < nFields; ++i) {
                    r.push_back(static_cast<char *>(row[i]));
                }

                mRecordSet.add(r);
            }

            // free memory
            mysql_free_result(res);
        }
    }

    return mRecordSet;
}


/**
 * Close the connection to the database.
 */
void
MySqlDataProvider::disconnect(void)
{
    if (!mIsConnected) {
        return;
    }

    // mysql_close() closes the connection and deallocates the connection
    // handle allocated by mysql_init().
    mysql_close(mDb);

    // deinitialize the MySQL client library.
    mysql_library_end();

    mDb = 0;
    mIsConnected = false;
}


} // namespace dal
} // namespace tmwserv
