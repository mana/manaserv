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


#if !defined (MYSQL_SUPPORT) && !defined (SQLITE_SUPPORT) && \
    !defined (POSTGRESQL_SUPPORT)

    // if we are in this block it means that we are not using a database
    // to persist the data from the storage.
    // at the moment, Storage assume that the data are persisted in a database
    // so let's raise a preprocessing error.
#error "no database backend defined"
#endif


#include <sstream>

#include <physfs.h>

#include "../utils/cipher.h"
#include "../utils/functors.h"
#include "../utils/logger.h"
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
    LOG("StorageTest::testGetAccount1()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    const std::string name("frodo");
    Account* account = myStorage.getAccount(name);

    using namespace tmwserv::utils;

    const std::string password(Cipher::instance().md5(name));
    const std::string email("frodo@domain");

    CPPUNIT_ASSERT(account != 0);
    CPPUNIT_ASSERT_EQUAL(name, account->getName());
    CPPUNIT_ASSERT_EQUAL(password, account->getPassword());
    CPPUNIT_ASSERT_EQUAL(email, account->getEmail());
}


/**
 * Fetch an unexisting account from the database.
 */
void
StorageTest::testGetAccount2(void)
{
    LOG("StorageTest::testGetAccount2()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    Account* account = myStorage.getAccount("xxx");

    CPPUNIT_ASSERT(account == 0);
}


/**
 * Test passing a null pointer to addAcccount().
 */
void
StorageTest::testAddAccount1(void)
{
    LOG("StorageTest::testAddAccount1()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    // TODO: when addAccount will throw exceptions, test the exceptions
    // thrown.
    // nothing should happen at the moment.
    AccountPtr nullAccount(0);
    myStorage.addAccount(nullAccount);
    myStorage.flush();

    // check the database.
    Checks checks;
    checks.set(CHK_DEFAULT_ACCOUNTS);
    checkDb(checks);
}


/**
 * Test adding a new account.
 */
void
StorageTest::testAddAccount2(void)
{
    LOG("StorageTest::testAddAccount2()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    // prepare new account.
    RawStatistics stats = {1, 1, 1, 1, 1, 1};

    BeingPtr b1(new Being("sam1", GENDER_MALE, 0, 0, stats));
    BeingPtr b2(new Being("sam2", GENDER_MALE, 0, 0, stats));
    Beings characters;
    characters.push_back(b1);
    characters.push_back(b2);

    const std::string sam("sam");
    AccountPtr acc(new Account(sam, sam, "sam@domain", characters));

    // TODO: when addAccount will throw exceptions, test the exceptions
    // thrown.
    myStorage.addAccount(acc);
    myStorage.flush();

    // check the database.
    Checks checks;
    checks.set(CHK_NEW_ADDED_ACCOUNT);
    checks.set(CHK_CHARACTERS_4);
    checkDb(checks);
}


/**
 * Test updating an existing account with new characters.
 */
void
StorageTest::testUpdAccount1(void)
{
    LOG("StorageTest::testUpdAccount1()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    // get an existing account.
    const std::string name("frodo");
    Account* account = myStorage.getAccount(name);

    // create new characters.
    RawStatistics stats = {1, 1, 1, 1, 1, 1};

    BeingPtr b1(new Being("sam1", GENDER_MALE, 0, 0, stats));
    BeingPtr b2(new Being("sam2", GENDER_MALE, 0, 0, stats));

    // add the characters to the account.
    account->addCharacter(b1);
    account->addCharacter(b2);

    // update the database.
    myStorage.flush();

    // check the database.
    Checks checks;
    checks.set(CHK_DEFAULT_ACCOUNTS);
    checks.set(CHK_CHARACTERS_1);
    checkDb(checks);
}


/**
 * Test updating an existing account with a new password and new
 * character stats.
 */
void
StorageTest::testUpdAccount2(void)
{
    LOG("StorageTest::testUpdAccount2()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    // get an existing account.
    const std::string name("frodo");
    Account* account = myStorage.getAccount(name);

    // create new characters.
    RawStatistics stats = {1, 1, 1, 1, 1, 1};

    BeingPtr b1(new Being("sam1", GENDER_MALE, 0, 0, stats));
    BeingPtr b2(new Being("sam2", GENDER_MALE, 0, 0, stats));

    // add the characters to the account.
    account->addCharacter(b1);
    account->addCharacter(b2);

    // update the database.
    myStorage.flush();

    // change the account password.
    using tmwserv::utils::Cipher;
    const std::string newPassword(Cipher::instance().md5("myprecious"));
    account->setPassword(newPassword);

    // change the strength of the first character.
    b1->setStrength(10);

    // update the database.
    myStorage.flush();

    // check the database.
    Checks checks;
    checks.set(CHK_DEFAULT_ACCOUNTS);
    checks.set(CHK_CHARACTERS_UPDATE_1);
    checkDb(checks);
}


/**
 * Test deleting an account that exists in the database but not
 * loaded yet in memory.
 */
void
StorageTest::testDelAccount1(void)
{
    LOG("StorageTest::testDelAccount1()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    myStorage.delAccount("frodo");

    // check the database.
    Checks checks;
    checks.set(CHK_1ST_ACCOUNT_DELETED);
    checkDb(checks);
}


/**
 * Test deleting an account that was added to the storage but not
 * yet persisted.
 */
void
StorageTest::testDelAccount2(void)
{
    LOG("StorageTest::testDelAccount2()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    // prepare new account.
    RawStatistics stats = {1, 1, 1, 1, 1, 1};

    BeingPtr b1(new Being("sam1", GENDER_MALE, 0, 0, stats));
    BeingPtr b2(new Being("sam2", GENDER_MALE, 0, 0, stats));
    Beings characters;
    characters.push_back(b1);
    characters.push_back(b2);

    const std::string sam("sam");
    AccountPtr acc(new Account(sam, sam, "sam@domain", characters));

    myStorage.addAccount(acc);
    myStorage.delAccount(sam);

    // nothing should be added to the database.
    myStorage.flush();

    // check the database.
    Checks checks;
    checks.set(CHK_DEFAULT_ACCOUNTS);
    checkDb(checks);
}


/**
 * Test deleting an account that exists in the database and loaded
 * in memory.
 */
void
StorageTest::testDelAccount3(void)
{
    LOG("StorageTest::testDelAccount3()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    // get an existing account.
    const std::string name("frodo");
    Account* account = myStorage.getAccount(name);

    CPPUNIT_ASSERT_EQUAL(name, account->getName());

    // update the account with new characters so that we can check
    // that delAccount() is doing more than just deleting the tmw_accounts
    // table.
    RawStatistics stats = {1, 1, 1, 1, 1, 1};
    BeingPtr b1(new Being("sam1", GENDER_MALE, 0, 0, stats));
    BeingPtr b2(new Being("sam2", GENDER_MALE, 0, 0, stats));
    account->addCharacter(b1);
    account->addCharacter(b2);

    // flush() so that the new characters are stored in the database
    // before we delete the account.
    myStorage.flush();

    // we won't check if the tmw_characters contains the newly added
    // characters as it has been tested in testUpdAccount2().

    // finally, we are ready to delete the account.
    myStorage.delAccount(name);
    myStorage.flush();

    // check the database.
    Checks checks;
    checks.set(CHK_1ST_ACCOUNT_DELETED);
    checks.set(CHK_NO_CHARACTERS_1);
    checkDb(checks);
}


/**
 * Test deleting an account that does not exist.
 */
void
StorageTest::testDelAccount4(void)
{
    LOG("StorageTest::testDelAccount4()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    // nothing should happen nor modified in the database.
    myStorage.delAccount("xxx");
    myStorage.flush();

    // check the database.
    Checks checks;
    checks.set(CHK_DEFAULT_ACCOUNTS);
    checkDb(checks);
}


/**
 * Test deleting twice an account that exists in the database and
 * loaded in memory.
 */
void
StorageTest::testDelAccount5(void)
{
    LOG("StorageTest::testDelAccount5()");

    Storage& myStorage = Storage::instance(mStorageName);

    CPPUNIT_ASSERT(myStorage.isOpen());

    // get an existing account.
    const std::string name("frodo");
    Account* account = myStorage.getAccount(name);

    CPPUNIT_ASSERT_EQUAL(name, account->getName());

    myStorage.delAccount(name);
    myStorage.flush();

    // delete it again, no error should occur.
    myStorage.delAccount(name);
    myStorage.flush();

    // check the database.
    Checks checks;
    checks.set(CHK_1ST_ACCOUNT_DELETED);
    checkDb(checks);
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


/**
 * Check the state of the database.
 */
void
StorageTest::checkDb(const Checks& what)
{
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

        if (what[CHK_DEFAULT_ACCOUNTS]) {
            CPPUNIT_ASSERT(rs.rows() == 3);

            const std::string frodo("frodo");
            const std::string merry("merry");
            const std::string pippin("pippin");

            CPPUNIT_ASSERT_EQUAL(frodo, rs(0, "username"));
            CPPUNIT_ASSERT_EQUAL(merry, rs(1, "username"));
            CPPUNIT_ASSERT_EQUAL(pippin, rs(2, "username"));
        }

        if (what[CHK_1ST_ACCOUNT_DELETED]) {
            CPPUNIT_ASSERT(rs.rows() == 2);

            const std::string merry("merry");
            const std::string pippin("pippin");

            CPPUNIT_ASSERT_EQUAL(merry, rs(0, "username"));
            CPPUNIT_ASSERT_EQUAL(pippin, rs(1, "username"));
        }

        if (what[CHK_NEW_ADDED_ACCOUNT]) {
            CPPUNIT_ASSERT(rs.rows() == 4);

            const std::string frodo("frodo");
            const std::string merry("merry");
            const std::string pippin("pippin");
            const std::string sam("sam");

            CPPUNIT_ASSERT_EQUAL(frodo, rs(0, "username"));
            CPPUNIT_ASSERT_EQUAL(merry, rs(1, "username"));
            CPPUNIT_ASSERT_EQUAL(pippin, rs(2, "username"));
            CPPUNIT_ASSERT_EQUAL(sam, rs(3, "username"));
        }

        if (what[CHK_CHARACTERS_1] || what[CHK_CHARACTERS_UPDATE_1] ||
            what[CHK_NO_CHARACTERS_1])
        {
            sql = "select * from ";
            sql += CHARACTERS_TBL_NAME;
            sql += " where user_id = 1;";
        }
        else if (what[CHK_CHARACTERS_4]) {
            sql = "select * from ";
            sql += CHARACTERS_TBL_NAME;
            sql += " where user_id = 4;";
        }

        db->execSql(sql);

        if (what[CHK_CHARACTERS_1] || what[CHK_CHARACTERS_4] ||
            what[CHK_CHARACTERS_UPDATE_1])
        {
            CPPUNIT_ASSERT(rs.rows() == 2);

            const std::string sam1("sam1");
            const std::string sam2("sam2");

            CPPUNIT_ASSERT_EQUAL(sam1, rs(0, "name"));
            CPPUNIT_ASSERT_EQUAL(sam2, rs(1, "name"));

            if (what[CHK_CHARACTERS_UPDATE_1]) {
                string_to<unsigned short> toUshort;
                CPPUNIT_ASSERT_EQUAL((unsigned short) 10,
                                     toUshort(rs(0, "str")));
            }
        }
        else if (what[CHK_NO_CHARACTERS_1]) {
            CPPUNIT_ASSERT(rs.rows() == 0);
        }

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
#endif
}
