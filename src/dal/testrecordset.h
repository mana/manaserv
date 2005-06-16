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


#ifndef _TMWSERV_TEST_RECORDSET_H_
#define _TMWSERV_TEST_RECORDSET_H_


#include <cppunit/extensions/HelperMacros.h>

#include "dalexcept.h"


/**
 * Unit test for the RecordSet class.
 */
class RecordSetTest: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(RecordSetTest);

    // add tests to the test suite.
    CPPUNIT_TEST(testRows1);
    CPPUNIT_TEST(testRows2);
    CPPUNIT_TEST(testCols1);
    CPPUNIT_TEST(testCols2);
    CPPUNIT_TEST(testIsEmpty1);
    CPPUNIT_TEST(testIsEmpty2);
    CPPUNIT_TEST_EXCEPTION(testOperator1, std::invalid_argument);
    CPPUNIT_TEST(testOperator2);
    CPPUNIT_TEST_EXCEPTION(testOperator3, std::out_of_range);
    CPPUNIT_TEST(testOperator4);
    CPPUNIT_TEST_EXCEPTION(testOperator5, std::out_of_range);
    CPPUNIT_TEST_EXCEPTION(testOperator6, std::invalid_argument);
    CPPUNIT_TEST(testOutputStream1);
    CPPUNIT_TEST(testOutputStream2);
    CPPUNIT_TEST(testAdd1);
    CPPUNIT_TEST_EXCEPTION(testAdd2, std::invalid_argument);
    CPPUNIT_TEST_EXCEPTION(testAdd3, tmwserv::dal::RsColumnHeadersNotSet);

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
         * Call RecordSet::rows() from an empty RecordSet.
         */
        void
        testRows1(void);


        /**
         * Call RecordSet::rows() from a non-empty RecordSet.
         */
        void
        testRows2(void);


        /**
         * Call RecordSet::cols() from an empty RecordSet.
         */
        void
        testCols1(void);


        /**
         * Call RecordSet::cols() from a non-empty RecordSet.
         */
        void
        testCols2(void);


        /**
         * Call RecordSet::isEmpty() from an empty RecordSet.
         */
        void
        testIsEmpty1(void);


        /**
         * Call RecordSet::isEmpty() from a non-empty RecordSet.
         */
        void
        testIsEmpty2(void);


        /**
         * Call RecordSet::operator() from an empty RecordSet.
         */
        void
        testOperator1(void);


        /**
         * Call RecordSet::operator() from a non-empty RecordSet with
         * a row index and a column index as parameters.
         */
        void
        testOperator2(void);

        /**
         * Call RecordSet::operator() from a non-empty RecordSet with
         * a row index and a column index that are out of range as
         * parameters.
         */
        void
        testOperator3(void);


        /**
         * Call RecordSet::operator() from a non-empty RecordSet with
         * a row index and a field name as parameters.
         */
        void
        testOperator4(void);


        /**
         * Call RecordSet::operator() from a non-empty RecordSet with
         * a row index that is out of range and a field name as parameters.
         */
        void
        testOperator5(void);


        /**
         * Call RecordSet::operator() from a non-empty RecordSet with
         * a row index and a field name that does not exist as parameters.
         */
        void
        testOperator6(void);


        /**
         * Test writing an empty RecordSet to an output stream.
         */
        void
        testOutputStream1(void);


        /**
         * Test writing a non-empty RecordSet to an output stream.
         */
        void
        testOutputStream2(void);


        /**
         * Call RecordSet::add() to add a new now.
         */
        void
        testAdd1(void);


        /**
         * Call RecordSet::add() to add a new now with a different number
         * of fields.
         */
        void
        testAdd2(void);


        /**
         * Call RecordSet::add() to add a new now to a RecordSet that does
         * not have column headers.
         */
        void
        testAdd3(void);


    private:
        tmwserv::dal::RecordSet mEmptyRs;    /**< empty recordset */
        tmwserv::dal::RecordSet mNonEmptyRs; /**< recordset with some data */
};


#endif // _TMWSERV_TEST_RECORDSET_H_
