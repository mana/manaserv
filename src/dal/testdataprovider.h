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



#ifndef _TMWSERV_TEST_DATA_PROVIDER_H_
#define _TMWSERV_TEST_DATA_PROVIDER_H_


#include <cppunit/extensions/HelperMacros.h>

#include "dalexcept.h"


/**
 * Unit test for the DataProvider class.
 */
class DataProviderTest: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(DataProviderTest);

    // add tests to the test suite.
    CPPUNIT_TEST(testConnection1);
    CPPUNIT_TEST(testCreateTable1);
    CPPUNIT_TEST_EXCEPTION(testCreateTable2,
                           tmwserv::dal::DbSqlQueryExecFailure);
    CPPUNIT_TEST(testInsert1);
    CPPUNIT_TEST_EXCEPTION(testInsert2, tmwserv::dal::DbSqlQueryExecFailure);
    CPPUNIT_TEST(testFetch1);
    CPPUNIT_TEST(testDisconnection1);
    CPPUNIT_TEST(testDisconnection2);

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
         * Connection to an existing database.
         */
        void
        testConnection1(void);


        /**
         * Create a new table in the database.
         */
        void
        testCreateTable1(void);


        /**
         * Create the same table one more time in the database.
         */
        void
        testCreateTable2(void);


        /**
         * Insert a new record into the table.
         */
        void
        testInsert1(void);


        /**
         * Insert the same record again.
         */
        void
        testInsert2(void);


        /**
         * Fetch data from the table.
         */
        void
        testFetch1(void);


        /**
         * Disconnection from an open database.
         */
        void
        testDisconnection1(void);


        /**
         * Disconnection from a closed database.
         */
        void
        testDisconnection2(void);


    private:
        tmwserv::dal::DataProvider* mDb; /**< the data provider */
        std::string mDbName;             /**< the database name */
        std::string mDbPath;             /**< the database path */
        std::string mDbUser;             /**< the database user */
        std::string mDbPassword;         /**< the database password */
        std::string mSqlCreateTable;     /**< SQL query to create table */
        std::string mSqlInsertRow;       /**< SQL query to delete table */
        std::string mSqlFetchRow;        /**< SQL query to fetch data */
};


#endif // _TMWSERV_TEST_DATA_PROVIDER_H_
