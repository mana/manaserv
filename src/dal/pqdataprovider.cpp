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


#include "pqdataprovider.h"


namespace tmwserv
{
namespace dal
{


/**
 * Constructor
 */
PqDataProvider::PqDataProvider(void)
    throw()
        : mDb(0)
{
    // NOOP
}


/**
 * Destructor
 */
PqDataProvider::~PqDataProvider(void)
    throw()
{
    if (mIsConnected) {
        disconnect();
    }
}


/**
 * Get the database backend name.
 */
DbBackends
PqDataProvider::getDbBackend(void) const
    throw()
{
    return DB_BKEND_POSTGRESQL;
}


/**
 * Create a connection to the database.
 */
void
PqDataProvider::connect(const std::string& dbName,
                        const std::string& userName,
                        const std::string& password)
{
    // Create string to pass to PQconnectdb
    std::string connStr = "dbname = " + dbName + " "; // database name
    if (userName != "")
        connStr += "user = " + userName + " "; // username
    if (password != "")
        connStr += "password = " + password; // password

    // Connect to database
    mDb = PQconnectdb(connStr.c_str());

    if (PQstatus(mDb) != CONNECTION_OK)
    {
        std::string error = PQerrorMessage(mDb);
        PQfinish(mDb);
        throw DbConnectionFailure(error);
    }

    // Save the Db Name.
    mDbName = dbName;

    mIsConnected = true;
}


/**
 * Execute a SQL query.
 */
const RecordSet&
PqDataProvider::execSql(const std::string& sql,
                        const bool refresh)
{
    if (!mIsConnected) {
        throw std::runtime_error("not connected to database");
    }

    if (refresh || (sql != mSql)) {
        mRecordSet.clear();

        // execute the query
        PGresult *res;

        res = PQexec(mDb, sql.c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            PQclear(res);
            throw DbSqlQueryExecFailure(PQerrorMessage(mDb));
        }

        // get field count
        unsigned int nFields = PQnfields(res);

        // fill column names
        Row fieldNames;
        for (unsigned int i = 0; i < nFields; i++) {
            fieldNames.push_back(PQfname(res, i));
        }
        mRecordSet.setColumnHeaders(fieldNames);

        // fill rows
        for (unsigned int r = 0; r < PQntuples(res); r++) {
            Row row;

            for (unsigned int i = 0; i < nFields; i++) {
                row.push_back(PQgetvalue(res, r, i));
            }

            mRecordSet.add(row);
        }

        // clear results
        PQclear(res);
    }
}


/**
 * Close connection to database.
 */
void
PqDataProvider::disconnect(void)
{
    if (!mIsConnected) {
        return;
    }

    // finish up with Postgre.
    PQfinish(mDb);

    mDb = 0;
    mIsConnected = false;
}


} // namespace dal
} // namespace tmwserv
