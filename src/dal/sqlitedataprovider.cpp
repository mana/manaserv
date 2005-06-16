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


#include "sqlitedataprovider.h"


namespace tmwserv
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
        // the memory allocated for the handle.
        disconnect();
    }
    catch (...) {
        // ignore
    }
}


/**
 * Get the name of the database backend.
 */
DbBackends
SqLiteDataProvider::getDbBackend(void) const
    throw()
{
    return DB_BKEND_SQLITE;
}


/**
 * Create a connection to the database.
 */
void
SqLiteDataProvider::connect(const std::string& dbName,
                            const std::string& userName,
                            const std::string& password)
{
    // sqlite3_open creates the database file if it does not exist
    // as a side-effect.
    if (sqlite3_open(dbName.c_str(), &mDb) != SQLITE_OK) {
        // save the error message thrown by sqlite3_open()
        // as we may lose it when sqlite3_close() runs.
        std::string msg(sqlite3_errmsg(mDb));

        // the SQLite3 documentation suggests that we try to close
        // the database after an unsuccessful call to sqlite3_open().
        sqlite3_close(mDb);

        throw DbConnectionFailure(msg);
    }

    mIsConnected = true;
}


/**
 * Execute a SQL query.
 */
const RecordSet&
SqLiteDataProvider::execSql(const std::string& sql,
                            const bool refresh)
{
    if (!mIsConnected) {
        throw std::runtime_error("not connected to database");
    }

    // do something only if the query is different from the previous
    // or if the cache must be refreshed
    // otherwise just return the recordset from cache.
    if (refresh || (sql != mSql)) {
        char** result;
        int nRows;
        int nCols;
        char* errMsg;

        mRecordSet.clear();

        int errCode = sqlite3_get_table(
  			              mDb,          // an open database
  			              sql.c_str(),  // SQL to be executed
                          &result,      // result of the query
                          &nRows,       // number of result rows
                          &nCols,       // number of result columns
                          &errMsg       // error msg
                      );

        if (errCode != SQLITE_OK) {
            throw DbSqlQueryExecFailure(sqlite3_errmsg(mDb));
        }

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
} // namespace tmwserv
