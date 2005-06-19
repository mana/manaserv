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


#ifndef _TMWSERV_TEST_CIPHER_H_
#define _TMWSERV_TEST_CIPHER_H_


#include <cppunit/extensions/HelperMacros.h>


/**
 * Unit test for the Cipher class.
 */
class CipherTest: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CipherTest);

    // add tests to the test suite.
    CPPUNIT_TEST(testMd5_1);
    CPPUNIT_TEST(testMd5_2);
    CPPUNIT_TEST(testMd5_3);
    CPPUNIT_TEST(testMd5_4);
    CPPUNIT_TEST(testMd5_5);
    CPPUNIT_TEST(testMd5_6);
    CPPUNIT_TEST(testMd5_7);

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
         * Test encoding a string with the MD5 digest algorithm.
         */
        void
        testMd5_1(void);


        /**
         * Test encoding a string with the MD5 digest algorithm.
         */
        void
        testMd5_2(void);


        /**
         * Test encoding a string with the MD5 digest algorithm.
         */
        void
        testMd5_3(void);


        /**
         * Test encoding a string with the MD5 digest algorithm.
         */
        void
        testMd5_4(void);


        /**
         * Test encoding a string with the MD5 digest algorithm.
         */
        void
        testMd5_5(void);


        /**
         * Test encoding a string with the MD5 digest algorithm.
         */
        void
        testMd5_6(void);


        /**
         * Test encoding a string with the MD5 digest algorithm.
         */
        void
        testMd5_7(void);
};


#endif // _TMWSERV_TEST_CIPHER_H_
