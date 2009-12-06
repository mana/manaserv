/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mysqldataprovider.h"

#include "dalexcept.h"

namespace dal
{

const std::string  MySqlDataProvider::CFGPARAM_MYSQL_HOST ="mysql_hostname";
const std::string  MySqlDataProvider::CFGPARAM_MYSQL_PORT ="mysql_port";
const std::string  MySqlDataProvider::CFGPARAM_MYSQL_DB   ="mysql_database";
const std::string  MySqlDataProvider::CFGPARAM_MYSQL_USER ="mysql_username";
const std::string  MySqlDataProvider::CFGPARAM_MYSQL_PWD  ="mysql_password";

const std::string  MySqlDataProvider::CFGPARAM_MYSQL_HOST_DEF = "localhost";
const unsigned int MySqlDataProvider::CFGPARAM_MYSQL_PORT_DEF = 3306;
const std::string  MySqlDataProvider::CFGPARAM_MYSQL_DB_DEF   = "mana";
const std::string  MySqlDataProvider::CFGPARAM_MYSQL_USER_DEF = "mana";
const std::string  MySqlDataProvider::CFGPARAM_MYSQL_PWD_DEF  = "mana";

/**
 * Constructor.
 */
MySqlDataProvider::MySqlDataProvider()
    throw()
        : mDb(0)
{
}


/**
 * Destructor.
 */
MySqlDataProvider::~MySqlDataProvider()
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
DbBackends MySqlDataProvider::getDbBackend() const
    throw()
{
    return DB_BKEND_MYSQL;
}


/**
 * Create a connection to the database.
 */
void MySqlDataProvider::connect()
{
    if (mIsConnected) {
        return;
    }

    // retrieve configuration from config file
    const std::string hostname
        = Configuration::getValue(CFGPARAM_MYSQL_HOST, CFGPARAM_MYSQL_HOST_DEF);
    const std::string dbName
        = Configuration::getValue(CFGPARAM_MYSQL_DB, CFGPARAM_MYSQL_DB_DEF);
    const std::string username
        = Configuration::getValue(CFGPARAM_MYSQL_USER, CFGPARAM_MYSQL_USER_DEF);
    const std::string password
        = Configuration::getValue(CFGPARAM_MYSQL_PWD, CFGPARAM_MYSQL_PWD_DEF);
    const unsigned int tcpPort
        = Configuration::getValue(CFGPARAM_MYSQL_PORT, CFGPARAM_MYSQL_PORT_DEF);

    // allocate and initialize a new MySQL object suitable
    // for mysql_real_connect().
    mDb = mysql_init(NULL);

    if (!mDb) {
        throw DbConnectionFailure(
            "unable to initialize the MySQL library: no memory");
    }

    LOG_INFO("Trying to connect with mySQL database server '"
        << hostname << ":" << tcpPort << "' using '" << username
        << "' as user, and '" << dbName << "' as database.");

    // actually establish the connection.
    if (!mysql_real_connect(mDb,                // handle to the connection
                            hostname.c_str(),   // hostname
                            username.c_str(),   // username
                            password.c_str(),   // password
                            dbName.c_str(),     // database name
                            tcpPort,            // tcp port
                            NULL,               // socket, currently not used
                            CLIENT_FOUND_ROWS)) // client flags
    {
        std::string msg(mysql_error(mDb));
        mysql_close(mDb);

        throw DbConnectionFailure(msg);
    }

    // Save the Db Name.
    mDbName = dbName;

    mIsConnected = true;
    LOG_INFO("Connection to mySQL was sucessfull.");
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

    LOG_DEBUG("Performing SQL query: "<<sql);

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
void MySqlDataProvider::disconnect()
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

void MySqlDataProvider::beginTransaction()
    throw (std::runtime_error)
{
    if (!mIsConnected)
    {
        const std::string error = "Trying to begin a transaction while not "
            "connected to the database!";
        LOG_ERROR(error);
        throw std::runtime_error(error);
    }

    mysql_autocommit(mDb, AUTOCOMMIT_OFF);
    execSql("BEGIN");
    LOG_DEBUG("SQL: started transaction");
}

void MySqlDataProvider::commitTransaction()
    throw (std::runtime_error)
{
    if (!mIsConnected)
    {
        const std::string error = "Trying to commit a transaction while not "
            "connected to the database!";
        LOG_ERROR(error);
        throw std::runtime_error(error);
    }

    if (mysql_commit(mDb) != 0)
    {
        LOG_ERROR("MySqlDataProvider::commitTransaction: " << mysql_error(mDb));
        throw DbSqlQueryExecFailure(mysql_error(mDb));
    }
    mysql_autocommit(mDb, AUTOCOMMIT_ON);
    LOG_DEBUG("SQL: commited transaction");
}

void MySqlDataProvider::rollbackTransaction()
    throw (std::runtime_error)
{
    if (!mIsConnected)
    {
        const std::string error = "Trying to rollback a transaction while not "
            "connected to the database!";
        LOG_ERROR(error);
        throw std::runtime_error(error);
    }

    if (mysql_rollback(mDb) != 0)
    {
        LOG_ERROR("MySqlDataProvider::rollbackTransaction: " << mysql_error(mDb));
        throw DbSqlQueryExecFailure(mysql_error(mDb));
    }
    mysql_autocommit(mDb, AUTOCOMMIT_ON);
    LOG_DEBUG("SQL: transaction rolled back");
}

unsigned MySqlDataProvider::getModifiedRows() const
{
    if (!mIsConnected)
    {
        const std::string error = "Trying to getModifiedRows while not "
            "connected to the database!";
        LOG_ERROR(error);
        throw std::runtime_error(error);
    }

    // FIXME: not sure if this is correct to bring 64bit int into int?
    const my_ulonglong affected = mysql_affected_rows(mDb);

    if (affected > INT_MAX)
        throw std::runtime_error("MySqlDataProvider::getLastId exceeded INT_MAX");

    if (affected == (my_ulonglong)-1)
    {
        LOG_ERROR("MySqlDataProvider::getModifiedRows: " << mysql_error(mDb));
        throw DbSqlQueryExecFailure(mysql_error(mDb));
    }

    return (unsigned) affected;
}

unsigned MySqlDataProvider::getLastId() const
{
    if (!mIsConnected)
    {
        const std::string error = "not connected to the database!";
        LOG_ERROR(error);
        throw std::runtime_error(error);
    }

    // FIXME: not sure if this is correct to bring 64bit int into int?
    const my_ulonglong lastId = mysql_insert_id(mDb);
    if (lastId > UINT_MAX)
        throw std::runtime_error("MySqlDataProvider::getLastId exceeded INT_MAX");

    return (unsigned) lastId;
}


} // namespace dal
