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

#include "recordset.h"
#include "testrecordset.h"


// register the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(RecordSetTest);


using namespace tmwserv::dal;


/**
 * Set up fixtures.
 */
void
RecordSetTest::setUp(void)
{
    // populate mNonEmptyRs.
    Row headers;
    headers.push_back("id");
    headers.push_back("name");
    mNonEmptyRs.setColumnHeaders(headers);

    Row r1;
    r1.push_back("1");
    r1.push_back("john");
    mNonEmptyRs.add(r1);

    Row r2;
    r2.push_back("2");
    r2.push_back("mike");
    mNonEmptyRs.add(r2);
}


/**
 * Tear down fixtures.
 */
void
RecordSetTest::tearDown(void)
{
    // NOOP
}


/**
 * Test RecordSet::rows() on an empty RecordSet.
 */
void
RecordSetTest::testRows1(void)
{
    CPPUNIT_ASSERT_EQUAL((unsigned int) 0, mEmptyRs.rows());
}


/**
 * Test RecordSet::rows() on a non-empty RecordSet.
 */
void
RecordSetTest::testRows2(void)
{
    CPPUNIT_ASSERT_EQUAL((unsigned int) 2, mNonEmptyRs.rows());
}


/**
 * Test RecordSet::cols() on an empty RecordSet.
 */
void
RecordSetTest::testCols1(void)
{
    CPPUNIT_ASSERT_EQUAL((unsigned int) 0, mEmptyRs.cols());
}


/**
 * Test RecordSet::cols() on a non-empty RecordSet.
 */
void
RecordSetTest::testCols2(void)
{
    CPPUNIT_ASSERT_EQUAL((unsigned int) 2, mNonEmptyRs.cols());
}


/**
 * Call RecordSet::isEmpty() from an empty RecordSet.
 */
void
RecordSetTest::testIsEmpty1(void)
{
    CPPUNIT_ASSERT(mEmptyRs.isEmpty());
}


/**
 * Call RecordSet::isEmpty() from a non-empty RecordSet.
 */
void
RecordSetTest::testIsEmpty2(void)
{
    CPPUNIT_ASSERT(!mNonEmptyRs.isEmpty());
}


/**
 * Call RecordSet::operator() from an empty RecordSet.
 */
void
RecordSetTest::testOperator1(void)
{
    // this should throw std::invalid_argument.
    mEmptyRs(0, 0);
}


/**
 * Call RecordSet::operator() from a non-empty RecordSet with
 * a row index and a column index as parameters.
 */
void
RecordSetTest::testOperator2(void)
{
    std::string value("mike");

    CPPUNIT_ASSERT_EQUAL(value, mNonEmptyRs(1, 1));
}


/**
 * Call RecordSet::operator() from a non-empty RecordSet with
 * a row index and a column index that are out of range as
 * parameters.
 */
void
RecordSetTest::testOperator3(void)
{
    // this should throw std::out_of_range.
    mNonEmptyRs(2, 2);
}


/**
 * Call RecordSet::operator() from a non-empty RecordSet with
 * a row index and a field name as parameters.
 */
void
RecordSetTest::testOperator4(void)
{
    std::string value("1");

    CPPUNIT_ASSERT_EQUAL(value, mNonEmptyRs(0, "id"));
}


/**
 * Call RecordSet::operator() from a non-empty RecordSet with
 * a row index that is out of range and a field name as parameters.
 */
void
RecordSetTest::testOperator5(void)
{
    // this should throw std::out_of_range.
    mNonEmptyRs(3, "id");
}


/**
 * Call RecordSet::operator() from a non-empty RecordSet with
 * a row index and a field name that does not exist as parameters.
 */
void
RecordSetTest::testOperator6(void)
{
    // this should throw std::invalid_argument.
    mNonEmptyRs(1, "noname");
}


/**
 * Test writing an empty RecordSet to an output stream.
 */
void
RecordSetTest::testOutputStream1(void)
{
    std::string emptyStr;

    std::ostringstream os;
    os << mEmptyRs;

    CPPUNIT_ASSERT_EQUAL(emptyStr, os.str());
}


/**
 * Test writing a non-empty RecordSet to an output stream.
 */
void
RecordSetTest::testOutputStream2(void)
{
    std::ostringstream os1;
    os1 << "|id|name|" << std::endl << std::endl
        << "|1|john|" << std::endl
        << "|2|mike|" << std::endl;

    std::ostringstream os2;
    os2 << mNonEmptyRs;

    CPPUNIT_ASSERT_EQUAL(os1.str(), os2.str());
}


/**
 * Test RecordSet::add() to add a new now.
 */
void
RecordSetTest::testAdd1(void)
{
    std::string id("3");
    std::string name("elena");

    Row r;
    r.push_back(id);
    r.push_back(name);
    mNonEmptyRs.add(r);

    CPPUNIT_ASSERT_EQUAL((unsigned int) 3, mNonEmptyRs.rows());
    CPPUNIT_ASSERT_EQUAL((unsigned int) 2, mNonEmptyRs.cols());
    CPPUNIT_ASSERT_EQUAL(id, mNonEmptyRs(2, 0));
    CPPUNIT_ASSERT_EQUAL(name, mNonEmptyRs(2, 1));
}


/**
 * Test RecordSet::add() to add a new now with a different number
 * of fields.
 */
void
RecordSetTest::testAdd2(void)
{
    Row r;
    r.push_back("4");

    // this should throw std::invalid_argument.
    mNonEmptyRs.add(r);
}


/**
 * Test RecordSet::add() to add a new now to a RecordSet that does
 * not have column headers.
 */
void
RecordSetTest::testAdd3(void)
{
    Row r;
    r.push_back("5");

    // this should throw tmw::dal::RsColumnHeadersNotSet.
    mEmptyRs.add(r);
}
