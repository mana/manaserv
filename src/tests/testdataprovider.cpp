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


#include <physfs.h>

#if defined (MYSQL_SUPPORT)
#include <mysql/mysql.h>
#elif defined (POSTGRE_SUPPORT)
#include <libpq-fe.h>
#endif

#include "../dal/dataproviderfactory.h"

#include "testdataprovider.h"


// register the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(DataProviderTest);


// initialize the static variables.
std::string DataProviderTest::mDbName("tmwteststorage");
std::string DataProviderTest::mDbUser("guest");
std::string DataProviderTest::mDbPassword("guest");
std::string DataProviderTest::mSqlCreateTable(
    "create table employees ("
    "    id integer primary key, "
    "    name varchar(32) not null);"
);
std::string DataProviderTest::mSqlInsertRow(
    "insert into employees values (1, 'john');"
);
std::string DataProviderTest::mSqlFetchRow("select * from employees;");


using namespace tmwserv::dal;


/**
 * Set up fixtures.
 */
void
DataProviderTest::setUp(void)
{
    // reinitialize the database before each test.
    reinitDb();

    // obtain a data provider.
    try {
        mDb = DataProviderFactory::createDataProvider();
    }
    catch (const std::runtime_error& e) {
        CPPUNIT_FAIL(e.what());
    }
}


/**
 * Tear down fixtures.
 */
void
DataProviderTest::tearDown(void)
{
    mDb->disconnect();

    delete mDb;
    mDb = 0;

    // clean the database after each test.
    reinitDb();
}


/**
 * Connection to an existing database.
 */
void
DataProviderTest::testConnection1(void)
{
#ifdef SQLITE_SUPPORT
    std::string dbFile(mDbName);
    dbFile += ".db";

    mDb->connect(dbFile, mDbUser, mDbPassword);
#else
    mDb->connect(mDbName, mDbUser, mDbPassword);
#endif

    CPPUNIT_ASSERT(mDb->isConnected());
}


/**
 * Create a new table in the database.
 */
void
DataProviderTest::testCreateTable1(void)
{
#ifdef SQLITE_SUPPORT
    std::string dbFile(mDbName);
    dbFile += ".db";

    mDb->connect(dbFile, mDbUser, mDbPassword);
#else
    mDb->connect(mDbName, mDbUser, mDbPassword);
#endif

    CPPUNIT_ASSERT(mDb->isConnected());

    const RecordSet& rs = mDb->execSql(mSqlCreateTable);
    CPPUNIT_ASSERT(rs.isEmpty());

    mDb->disconnect();
    CPPUNIT_ASSERT(!mDb->isConnected());
}


/**
 * Create the same table one more time in the database.
 */
void
DataProviderTest::testCreateTable2(void)
{
#ifdef SQLITE_SUPPORT
    std::string dbFile(mDbName);
    dbFile += ".db";

    mDb->connect(dbFile, mDbUser, mDbPassword);
#else
    mDb->connect(mDbName, mDbUser, mDbPassword);
#endif

    CPPUNIT_ASSERT(mDb->isConnected());

    mDb->execSql(mSqlCreateTable);
    // this should throw tmwserv::dal::DbSqlQueryExecFailure.
    mDb->execSql(mSqlCreateTable);
}


/**
 * Insert a new row to the table.
 */
void
DataProviderTest::testInsert1(void)
{
#ifdef SQLITE_SUPPORT
    std::string dbFile(mDbName);
    dbFile += ".db";

    mDb->connect(dbFile, mDbUser, mDbPassword);
#else
    mDb->connect(mDbName, mDbUser, mDbPassword);
#endif

    CPPUNIT_ASSERT(mDb->isConnected());

    mDb->execSql(mSqlCreateTable);

    const RecordSet& rs = mDb->execSql(mSqlInsertRow);
    // an insert query does not return any records
    // so the recordset remains empty.
    CPPUNIT_ASSERT(rs.isEmpty());

    mDb->disconnect();
    CPPUNIT_ASSERT(!mDb->isConnected());
}


/**
 * Insert the same record again.
 */
void
DataProviderTest::testInsert2(void)
{
#ifdef SQLITE_SUPPORT
    std::string dbFile(mDbName);
    dbFile += ".db";

    mDb->connect(dbFile, mDbUser, mDbPassword);
#else
    mDb->connect(mDbName, mDbUser, mDbPassword);
#endif

    CPPUNIT_ASSERT(mDb->isConnected());

    // this should throw tmwserv::dal::DbSqlQueryExecFailure
    // as we are violating the primary key uniqueness.
    mDb->execSql(mSqlInsertRow);
}


/**
 * Fetch data from the table.
 */
void
DataProviderTest::testFetch1(void)
{
#ifdef SQLITE_SUPPORT
    std::string dbFile(mDbName);
    dbFile += ".db";

    mDb->connect(dbFile, mDbUser, mDbPassword);
#else
    mDb->connect(mDbName, mDbUser, mDbPassword);
#endif

    CPPUNIT_ASSERT(mDb->isConnected());

    mDb->execSql(mSqlCreateTable);
    mDb->execSql(mSqlInsertRow);

    const RecordSet& rs = mDb->execSql(mSqlFetchRow);
    CPPUNIT_ASSERT(!rs.isEmpty());

    std::string id("1");
    std::string name("john");
    CPPUNIT_ASSERT_EQUAL(id, rs(0, "id"));
    CPPUNIT_ASSERT_EQUAL(name, rs(0, "name"));

    mDb->disconnect();
    CPPUNIT_ASSERT(!mDb->isConnected());
}


/**
 * Disconnection from an open database.
 */
void
DataProviderTest::testDisconnection1(void)
{
#ifdef SQLITE_SUPPORT
    std::string dbFile(mDbName);
    dbFile += ".db";

    mDb->connect(dbFile, mDbUser, mDbPassword);
#else
    mDb->connect(mDbName, mDbUser, mDbPassword);
#endif

    CPPUNIT_ASSERT(mDb->isConnected());

    mDb->disconnect();
    CPPUNIT_ASSERT(!mDb->isConnected());
}


/**
 * Disconnection from a closed database.
 */
void
DataProviderTest::testDisconnection2(void)
{
    mDb->disconnect();
    CPPUNIT_ASSERT(!mDb->isConnected());
}


/**
 * Reinitialize the database.
 */
void
DataProviderTest::reinitDb(void)
{
    // we are not using DataProvider::execSql() to execute the SQL queries
    // here as the class is the purpose of these tests.

#if defined (MYSQL_SUPPORT)
    {
        // initialize the MySQL library.
        MYSQL* db = mysql_init(NULL);

        if (!db) {
            CPPUNIT_FAIL("unable to initialize the MySQL library: no memory");
        }

        // connect to the database.
        if (!mysql_real_connect(db,
                                NULL,
                                mDbUser.c_str(),
                                mDbPassword.c_str(),
                                mDbName.c_str(),
                                0,
                                NULL,
                                0))
        {
            CPPUNIT_FAIL(mysql_error(db));
        }

        // drop the table.
        std::string sql("drop table employees;");
        if (mysql_query(db, sql.c_str()) != 0) {
            // ignore, the table may not exist.
        }

        // close the connection and free memory.
        mysql_close(db);
        mysql_library_end();
        db = 0;
    }
#elif defined (POSTGRE_SUPPORT)
    // TODO: drop tables.
#else // SQLITE_SUPPORT
    std::string dbFile(mDbName);
    dbFile += ".db";

    // ensure that the database file does not exist.
    if (PHYSFS_exists(dbFile.c_str())) {
        if (PHYSFS_delete(dbFile.c_str()) == 0) {
            CPPUNIT_FAIL(PHYSFS_getLastError());
        }
    }
#endif
}
