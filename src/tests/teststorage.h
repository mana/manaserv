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


#ifndef _TMWSERV_TEST_STORAGE_H_
#define _TMWSERV_TEST_STORAGE_H_


#include <cppunit/extensions/HelperMacros.h>

#include "../dal/dataproviderfactory.h"


/**
 * Requirements:
 *     - if using MySQL or PostgreSQL as backends, then make sure that a user
 *       named 'guest' with the password 'guest' has enough privileges to
 *       create tables and populate them in a database named 'tmwteststorage'
 *       prior to running the teststorage unit tests.
 */


using namespace tmwserv::dal;


/**
 * Unit test for the Storage class.
 */
class StorageTest: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(StorageTest);

    // add tests to the test suite.
    CPPUNIT_TEST(testGetAccount1);
    CPPUNIT_TEST(testGetAccount2);

    CPPUNIT_TEST_SUITE_END();


    public:
        /**
         * Set up fixtures.
         */
        void
        setUp(void);


        /**
         * Tear down fixtures.
         */
        void
        tearDown(void);


        /**
         * Fetch an existing account from the database.
         */
        void
        testGetAccount1(void);


        /**
         * Fetch an unexisting account from the database.
         */
        void
        testGetAccount2(void);


    private:
        /**
         * Initialize the storage.
         */
        void
        initStorage(void);


        /**
         * Clean the storage.
         */
        void
        cleanStorage(void);


        /**
         * Drop a table.
         *
         * @param db the database.
         * @param name the table name.
         */
        void
        dropTable(std::auto_ptr<DataProvider>& db,
                  const std::string& name);


        /**
         * Insert a new account.
         *
         * @param db the database.
         * @param name the user name.
         */
        void
        insertAccount(std::auto_ptr<DataProvider>&,
                      const std::string& name);


    private:
        static std::string mStorageName;
        static std::string mStorageUser;
        static std::string mStorageUserPassword;
};


#endif // _TMWSERV_TEST_STORAGE_H_
