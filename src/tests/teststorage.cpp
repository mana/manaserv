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


#include <sstream>

#include <physfs.h>

#include "../utils/cipher.h"

#include "../dalstoragesql.h"
#include "../storage.h"
#include "teststorage.h"


// register the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(StorageTest);


// initialization of static attributes.
std::string StorageTest::mStorageName("tmwteststorage");
std::string StorageTest::mStorageUser("guest");
std::string StorageTest::mStorageUserPassword("guest");


using namespace tmwserv;


/**
 * Set up fixtures.
 */
void
StorageTest::setUp(void)
{
    // reinitialize the storage before each test.
    initStorage();

    // create a storage.
    Storage& myStorage = Storage::instance(mStorageName);
    myStorage.setUser(mStorageUser);
    myStorage.setPassword(mStorageUserPassword);

    // open the storage.
    myStorage.open();
}


/**
 * Tear down fixtures.
 */
void
StorageTest::tearDown(void)
{
    // close the storage.
    Storage& myStorage = Storage::instance(mStorageName);
    myStorage.close();

    // delete the storage.
    Storage::destroy();

    // clean the storage after each test.
    cleanStorage();
}


/**
 * Fetch an existing account from the database.
 */
void
StorageTest::testGetAccount1(void)
{
    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    Account* account = myStorage.getAccount("kindjal");

    using namespace tmwserv::utils;

    std::string name("kindjal");
    std::string password(Cipher::instance().md5("kindjal"));
    std::string email("kindjal@domain");

    CPPUNIT_ASSERT(account != 0);
    CPPUNIT_ASSERT_EQUAL(account->getName(), name);
    CPPUNIT_ASSERT_EQUAL(account->getPassword(), password);
    CPPUNIT_ASSERT_EQUAL(account->getEmail(), email);
}


/**
 * Fetch an unexisting account from the database.
 */
void
StorageTest::testGetAccount2(void)
{
    Storage& myStorage = Storage::instance(mStorageName);

    if (!myStorage.isOpen()) {
        CPPUNIT_FAIL("the storage is not opened.");
    }

    Account* account = myStorage.getAccount("xxx");

    CPPUNIT_ASSERT(account == 0);
}


/**
 * Initialize the storage.
 */
void
StorageTest::initStorage(void)
{
#if defined (MYSQL_SUPPORT) || defined (POSTGRE_SUPPORT) || \
    defined (SQLITE_SUPPORT)

    // we are using a database to persist the data from the storage.

    using namespace tmwserv::dal;

    // insert initial data using the data provider directly.
    // we must avoid using the APIs from Storage here as it's the purpose
    // of these tests.
    std::auto_ptr<DataProvider> db(DataProviderFactory::createDataProvider());

    try {
#ifdef SQLITE_SUPPORT
        std::string dbFile(mStorageName);
        dbFile += ".db";

        // ensure that the file does not exist before the tests begin.
        if (PHYSFS_exists(dbFile.c_str())) {
            if (PHYSFS_delete(dbFile.c_str()) == 0) {
                CPPUNIT_FAIL(PHYSFS_getLastError());
            }
        }

        db->connect(dbFile, mStorageUser, mStorageUserPassword);
#else
        db->connect(mStorageName, mStorageUser, mStorageUserPassword);
#endif

        // drop the tables.
        dropTable(db, MAPS_TBL_NAME);
        dropTable(db, ACCOUNTS_TBL_NAME);
        dropTable(db, CHARACTERS_TBL_NAME);
        dropTable(db, ITEMS_TBL_NAME);
        dropTable(db, WORLD_ITEMS_TBL_NAME);
        dropTable(db, INVENTORIES_TBL_NAME);

        // recreate the tables.
        db->execSql(SQL_MAPS_TABLE);
        db->execSql(SQL_ACCOUNTS_TABLE);
        db->execSql(SQL_CHARACTERS_TABLE);
        db->execSql(SQL_ITEMS_TABLE);
        db->execSql(SQL_WORLD_ITEMS_TABLE);
        db->execSql(SQL_INVENTORIES_TABLE);

        // populate the tables.
        insertAccount(db, "kindjal");

        db->disconnect();
    }
    catch (const DbConnectionFailure& e) {
        CPPUNIT_FAIL(e.what());
    }
    catch (const DbSqlQueryExecFailure& e) {
        CPPUNIT_FAIL(e.what());
    }
    catch (const DbDisconnectionFailure& e) {
        CPPUNIT_FAIL(e.what());
    }
    catch (const std::exception& e) {
        CPPUNIT_FAIL(e.what());
    }
    catch (...) {
        CPPUNIT_FAIL("unexpected exception");
    }
#else
    // if we are in this block it means that we are not using a database
    // to persist the data from the storage.
    // at the moment, Storage assume that the data are persisted in a database
    // so let's raise a preprocessing error.
#error "no database backend defined"
#endif
}


/**
 * Clean the storage.
 */
void
StorageTest::cleanStorage(void)
{
#if defined (MYSQL_SUPPORT) || defined (POSTGRE_SUPPORT) || \
    defined (SQLITE_SUPPORT)

    // we are using a database to persist the data from the storage.

    using namespace tmwserv::dal;

    std::auto_ptr<DataProvider> db(DataProviderFactory::createDataProvider());

    try {
#ifdef SQLITE_SUPPORT
        std::string dbFile(mStorageName);
        dbFile += ".db";

        // ensure that the file does not exist before the tests begin.
        if (PHYSFS_exists(dbFile.c_str())) {
            if (PHYSFS_delete(dbFile.c_str()) == 0) {
                CPPUNIT_FAIL(PHYSFS_getLastError());
            }
        }
#else
        db->connect(mStorageName, mStorageUser, mStorageUserPassword);

        // drop the tables.
        dropTable(db, MAPS_TBL_NAME);
        dropTable(db, ACCOUNTS_TBL_NAME);
        dropTable(db, CHARACTERS_TBL_NAME);
        dropTable(db, ITEMS_TBL_NAME);
        dropTable(db, WORLD_ITEMS_TBL_NAME);
        dropTable(db, INVENTORIES_TBL_NAME);

        db->disconnect();
#endif
    }
    catch (const DbConnectionFailure& e) {
        CPPUNIT_FAIL(e.what());
    }
    catch (const DbSqlQueryExecFailure& e) {
        CPPUNIT_FAIL(e.what());
    }
    catch (const DbDisconnectionFailure& e) {
        CPPUNIT_FAIL(e.what());
    }
    catch (const std::exception& e) {
        CPPUNIT_FAIL(e.what());
    }
    catch (...) {
        CPPUNIT_FAIL("unexpected exception");
    }
#else
    // if we are in this block it means that we are not using a database
    // to persist the data from the storage.
    // at the moment, Storage assume that the data are persisted in a database
    // so let's raise a preprocessing error.
#error "no database backend defined"
#endif
}


/**
 * Drop a table.
 */
void
StorageTest::dropTable(std::auto_ptr<DataProvider>& db,
                       const std::string& name)
{
    try {
        std::string sql("drop table ");
        sql += name;
        sql += ";";
        db->execSql(sql);
    }
    catch (const DbSqlQueryExecFailure& e) {
        // ignore, the table may not exist.
    }
}


/**
 * Insert a new account.
 */
void
StorageTest::insertAccount(std::auto_ptr<DataProvider>& db,
                           const std::string& name)
{
    std::ostringstream sql;

    // the password will be identical to the name.

    sql << "insert into " << ACCOUNTS_TBL_NAME
        << "(username, password, email, level, banned) values "
        << "('" << name << "', '" << name << "', '"
        << name << "@domain', 1, 0);";

    db->execSql(sql.str());
}
