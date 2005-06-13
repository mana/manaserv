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


namespace tmw
{
namespace dal
{


/**
 * Constructor.
 */
MySqlDataProvider::MySqlDataProvider(void)
    throw()
{
    // NOOP
}


/**
 * Destructor.
 */
MySqlDataProvider::~MySqlDataProvider(void)
    throw()
{
    // NOOP
}


/**
 * Get the database backend name.
 */
DbBackends
MySqlDataProvider::getDbBackend(void) const
    throw()
{
    return MYSQL;
}


/**
 * Create a new database.
 */
void
MySqlDataProvider::createDb(const std::string& dbName,
                            const std::string& dbPath)
    throw(DbCreationFailure,
          std::exception)
{
    // TODO
}


/**
 * Create a connection to the database.
 */
void
MySqlDataProvider::connect(const std::string& dbName,
                           const std::string& userName,
                           const std::string& password)
    throw(DbConnectionFailure,
          std::exception)
{
    // TODO
}


/**
 * Execute a SQL query.
 */
const RecordSet&
MySqlDataProvider::execSql(const std::string& sql)
    throw(DbSqlQueryExecFailure,
          std::exception)
{
    // do something only if the query is different from the previous
    // otherwise just return the recordset from cache.
    if (sql != mSql) {
        // TODO
    }

    return mRecordSet;
}


/**
 * Close the connection to the database.
 */
void
MySqlDataProvider::disconnect(void)
    throw(DbDisconnectionFailure,
          std::exception)
{
    // TODO
}


} // namespace dal
} // namespace tmw
