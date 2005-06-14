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


#include <iostream>

#include "sqlitedataprovider.h"


namespace tmw
{
namespace dal
{


/**
 * Constructor.
 */
SqLiteDataProvider::SqLiteDataProvider(void)
    throw()
        : mDb(0)
{
    // NOOP
}


/**
 * Destructor.
 */
SqLiteDataProvider::~SqLiteDataProvider(void)
    throw()
{
    try {
        // make sure that the database is closed.
        // disconnect() calls sqlite3_close() which takes care of freeing
        // the memory held by the class attribute mDb.
        disconnect();
    }
    catch (const DbDisconnectionFailure& e) {
        // TODO
    }
}


/**
 * Get the database backend name.
 */
DbBackends
SqLiteDataProvider::getDbBackend(void) const
    throw()
{
    return SQLITE;
}


/**
 * Create a new database.
 */
void
SqLiteDataProvider::createDb(const std::string& dbName,
                             const std::string& dbPath)
    throw(DbCreationFailure,
          std::exception)
{
    // TODO: handle dbPath

    // sqlite3_open creates the database file if it does not exist
    // and hence createDb is not very useful here.
    if (sqlite3_open(dbName.c_str(), &mDb) != SQLITE_OK) {
        throw DbCreationFailure(sqlite3_errmsg(mDb));
    }

    // nothing else to do, close the database.
    if (sqlite3_close(mDb) != SQLITE_OK) {
        throw DbCreationFailure(sqlite3_errmsg(mDb));
    }
}


/**
 * Create a connection to the database.
 */
void
SqLiteDataProvider::connect(const std::string& dbName,
                            const std::string& userName,
                            const std::string& password)
    throw(DbConnectionFailure,
          std::exception)
{
    connect(dbName, "", userName, password);
}


/**
 * Create a connection to the database.
 */
void
SqLiteDataProvider::connect(const std::string& dbName,
                            const std::string& dbPath,
                            const std::string& userName,
                            const std::string& password)
    throw(DbConnectionFailure,
          std::exception)
{
    // TODO: handle dbPath

    // sqlite3_open creates the database file if it does not exist.
    if (sqlite3_open(dbName.c_str(), &mDb) != SQLITE_OK) {
        // there is no need to check for the error code returned by
        // sqlite3_close() as we are going to throw an exception anyway
        sqlite3_close(mDb);

        throw DbConnectionFailure(sqlite3_errmsg(mDb));
    }

    mIsConnected = true;
}


/**
 * Execute a SQL query.
 */
const RecordSet&
SqLiteDataProvider::execSql(const std::string& sql,
                            const bool refresh)
    throw(DbSqlQueryExecFailure,
          std::exception)
{
    // do something only if the query is different from the previous
    // or if the cache must be refreshed
    // otherwise just return the recordset from cache.
    if (refresh || (sql != mSql)) {
        char** result;
        int nRows;
        int nCols;
        char* errMsg;

        int errCode = sqlite3_get_table(
  			              mDb,          // an open database
  			              sql.c_str(),  // SQL to be executed
                          &result,      // result of the query
                          &nRows,       // number of result rows
                          &nCols,       // number of result columns
                          &errMsg       // error msg
                      );

        if (errCode != SQLITE_OK) {
            std::cout << sqlite3_errmsg(mDb) << std::endl;
            throw DbSqlQueryExecFailure();
        }

        mRecordSet.clear();

        // the first row of result[] contains the field names.
        Row fieldNames;
        for(int col = 0; col < nCols; ++col) {
            fieldNames.push_back(result[col]);
        }
        mRecordSet.setColumnHeaders(fieldNames);

        // populate the RecordSet
        for (int row = 0; row < nRows; ++row) {
            Row r;

            for(int col = 0; col < nCols; ++col) {
                r.push_back(result[nCols + (row * nCols) + col]);

            }

            mRecordSet.add(r);
        }

        // free memory
        sqlite3_free_table(result);
        delete errMsg;
    }

    return mRecordSet;
}


/**
 * Close the connection to the database.
 */
void
SqLiteDataProvider::disconnect(void)
    throw(DbDisconnectionFailure,
          std::exception)
{
    if (!isConnected()) {
        return;
    }

    if (sqlite3_close(mDb) != SQLITE_OK) {
        throw DbDisconnectionFailure(sqlite3_errmsg(mDb));
    }

    mIsConnected = false;
}


} // namespace dal
} // namespace tmw
