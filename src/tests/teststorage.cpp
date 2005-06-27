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

    std::string name("frodo");
    Account* account = myStorage.getAccount(name);

    using namespace tmwserv::utils;

    std::string password(Cipher::instance().md5(name));
    std::string email("frodo@domain");

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
 * Test passing a null pointer to addAcccount().
 */
void
StorageTest::testAddAccount1(void)
{
    Storage& myStorage = Storage::instance(mStorageName);

    if (!myStorage.isOpen()) {
        CPPUNIT_FAIL("the storage is not opened.");
    }

    // TODO: when addAccount will throw exceptions, test the exceptions
    // thrown.
    // nothing should happen at the moment.
    myStorage.addAccount(NULL);
    myStorage.flush();

#if defined (MYSQL_SUPPORT) || defined (POSTGRESQL_SUPPORT) || \
    defined (SQLITE_SUPPORT)

    using namespace tmwserv::dal;

    std::auto_ptr<DataProvider> db(DataProviderFactory::createDataProvider());

    try {
#ifdef SQLITE_SUPPORT
        std::string dbFile(mStorageName);
        dbFile += ".db";
        db->connect(dbFile, mStorageUser, mStorageUserPassword);
#else
        db->connect(mStorageName, mStorageUser, mStorageUserPassword);
#endif

        std::string sql("select * from ");
        sql += ACCOUNTS_TBL_NAME;
        sql += ";";
        const RecordSet& rs = db->execSql(sql);

        CPPUNIT_ASSERT(rs.rows() == 3);

        std::string frodo("frodo");
        std::string merry("merry");
        std::string pippin("pippin");

        CPPUNIT_ASSERT_EQUAL(rs(0, "username"), frodo);
        CPPUNIT_ASSERT_EQUAL(rs(1, "username"), merry);
        CPPUNIT_ASSERT_EQUAL(rs(2, "username"), pippin);

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
 * Test adding a new account.
 */
void
StorageTest::testAddAccount2(void)
{
    Storage& myStorage = Storage::instance(mStorageName);

    if (!myStorage.isOpen()) {
        CPPUNIT_FAIL("the storage is not opened.");
    }

    // prepare new account.
    std::string sam1("sam1");
    std::string sam2("sam2");
    Being* b1 = new Being(sam1, 1, 1, 1, 1, 1, 1, 1, 1);
    Being* b2 = new Being(sam2, 1, 1, 1, 1, 1, 1, 1, 1);
    Beings characters;
    characters.push_back(b1);
    characters.push_back(b2);

    std::string sam("sam");
    Account* acc = new Account(sam, sam, "sam@domain", characters);

    // TODO: when addAccount will throw exceptions, test the exceptions
    // thrown.
    myStorage.addAccount(acc);
    myStorage.flush();

#if defined (MYSQL_SUPPORT) || defined (POSTGRESQL_SUPPORT) || \
    defined (SQLITE_SUPPORT)

    using namespace tmwserv::dal;

    std::auto_ptr<DataProvider> db(DataProviderFactory::createDataProvider());

    try {
#ifdef SQLITE_SUPPORT
        std::string dbFile(mStorageName);
        dbFile += ".db";
        db->connect(dbFile, mStorageUser, mStorageUserPassword);
#else
        db->connect(mStorageName, mStorageUser, mStorageUserPassword);
#endif

        std::string sql("select * from ");
        sql += ACCOUNTS_TBL_NAME;
        sql += ";";
        const RecordSet& rs = db->execSql(sql);

        CPPUNIT_ASSERT(rs.rows() == 4);

        std::string frodo("frodo");
        std::string merry("merry");
        std::string pippin("pippin");

        CPPUNIT_ASSERT_EQUAL(rs(0, "username"), frodo);
        CPPUNIT_ASSERT_EQUAL(rs(1, "username"), merry);
        CPPUNIT_ASSERT_EQUAL(rs(2, "username"), pippin);
        CPPUNIT_ASSERT_EQUAL(rs(3, "username"), sam);

        sql = "select * from ";
        sql += CHARACTERS_TBL_NAME;
        sql += " where user_id = '";
        sql += sam;
        sql += "';";

        db->execSql(sql);

        CPPUNIT_ASSERT(rs.rows() == 2);

        CPPUNIT_ASSERT_EQUAL(rs(0, "name"), sam1);
        CPPUNIT_ASSERT_EQUAL(rs(1, "name"), sam2);

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
 * Test updating an existing account.
 */
void
StorageTest::testUpdAccount1(void)
{
    // TODO
}


/**
 * Initialize the storage.
 */
void
StorageTest::initStorage(void)
{
#if defined (MYSQL_SUPPORT) || defined (POSTGRESQL_SUPPORT) || \
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
        insertAccount(db, "frodo");
        insertAccount(db, "merry");
        insertAccount(db, "pippin");

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
#if defined (MYSQL_SUPPORT) || defined (POSTGRESQL_SUPPORT) || \
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

    sql << "insert into " << ACCOUNTS_TBL_NAME << " values "
        << "(null, '" << name << "', '"
        << tmwserv::utils::Cipher::instance().md5(name) << "', '"
        << name << "@domain', 0, 0);";

    db->execSql(sql.str());
}
