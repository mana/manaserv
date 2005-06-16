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


#include "dataproviderfactory.h"
#include "testdataprovider.h"


// register the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(DataProviderTest);


using namespace tmwserv::dal;


/**
 * Set up fixtures.
 */
void
DataProviderTest::setUp(void)
{
    // obtain a data provider.
    try {
        mDb = DataProviderFactory::createDataProvider();
    }
    catch (const std::runtime_error& e) {
        CPPUNIT_FAIL(e.what());
    }

    // init db info and account.
#ifdef SQLITE_SUPPORT
    mDbName = "mydb.db";
#else
    mDbName = "mydb";
#endif
    mDbUser = "guest";
    mDbPassword = "guest";

    // init SQL queries.
    mSqlCreateTable  = "create table employees (";
    mSqlCreateTable += "    id int primary key, ";
    mSqlCreateTable += "    name varchar(32) not null);";

    mSqlInsertRow = "insert into employees values (1, 'john');";

    mSqlFetchRow = "select * from employees;";
}


/**
 * Tear down fixtures.
 */
void
DataProviderTest::tearDown(void)
{
    delete mDb;
}


/**
 * Connection to an existing database.
 */
void
DataProviderTest::testConnection1(void)
{
    mDb->connect(mDbName, mDbUser, mDbPassword);
    CPPUNIT_ASSERT(mDb->isConnected());
}


/**
 * Create a new table in the database.
 */
void
DataProviderTest::testCreateTable1(void)
{
    mDb->connect(mDbName, mDbUser, mDbPassword);
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
    mDb->connect(mDbName, mDbUser, mDbPassword);
    CPPUNIT_ASSERT(mDb->isConnected());

    // this should throw tmwserv::dal::DbSqlQueryExecFailure.
    const RecordSet& rs = mDb->execSql(mSqlCreateTable);
}


/**
 * Insert a new row to the table.
 */
void
DataProviderTest::testInsert1(void)
{
    mDb->connect(mDbName, mDbUser, mDbPassword);
    CPPUNIT_ASSERT(mDb->isConnected());

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
    mDb->connect(mDbName, mDbUser, mDbPassword);
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
    mDb->connect(mDbName, mDbUser, mDbPassword);
    CPPUNIT_ASSERT(mDb->isConnected());

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
    mDb->connect(mDbName, mDbUser, mDbPassword);
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
