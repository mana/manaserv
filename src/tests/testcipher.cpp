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


#include <string>

#include "../utils/cipher.h"
#include "testcipher.h"


// register the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(CipherTest);


using namespace tmwserv::utils;


/**
 * Set up fixtures.
 */
void
CipherTest::setUp(void)
{
    // NOOP
}


/**
 * Tear down fixtures.
 */
void
CipherTest::tearDown(void)
{
    // NOOP
}


/**
 * Test encoding a string with the MD5 digest algorithm.
 */
void
CipherTest::testMd5_1(void)
{
    const std::string expected("d41d8cd98f00b204e9800998ecf8427e");
    std::string actual(Cipher::instance().md5(""));

    CPPUNIT_ASSERT_EQUAL(expected, actual);
}


/**
 * Test encoding a string with the MD5 digest algorithm.
 */
void
CipherTest::testMd5_2(void)
{
    const std::string expected("0cc175b9c0f1b6a831c399e269772661");
    std::string actual(Cipher::instance().md5("a"));

    CPPUNIT_ASSERT_EQUAL(expected, actual);
}


/**
 * Test encoding a string with the MD5 digest algorithm.
 */
void
CipherTest::testMd5_3(void)
{
    const std::string expected("900150983cd24fb0d6963f7d28e17f72");
    std::string actual(Cipher::instance().md5("abc"));

    CPPUNIT_ASSERT_EQUAL(expected, actual);
}


/**
 * Test encoding a string with the MD5 digest algorithm.
 */
void
CipherTest::testMd5_4(void)
{
    const std::string expected("f96b697d7cb7938d525a2f31aaf161d0");
    std::string actual(Cipher::instance().md5("message digest"));

    CPPUNIT_ASSERT_EQUAL(expected, actual);
}


/**
 * Test encoding a string with the MD5 digest algorithm.
 */
void
CipherTest::testMd5_5(void)
{
    const std::string expected("c3fcd3d76192e4007dfb496cca67e13b");
    std::string actual(Cipher::instance().md5("abcdefghijklmnopqrstuvwxyz"));

    CPPUNIT_ASSERT_EQUAL(expected, actual);
}


/**
 * Test encoding a string with the MD5 digest algorithm.
 */
void
CipherTest::testMd5_6(void)
{
    const std::string expected("d174ab98d277d9f5a5611c2c9f419d9f");

    std::string s("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    s += "abcdefghijklmnopqrstuvwxyz";
    s += "0123456789";
    std::string actual(Cipher::instance().md5(s));

    CPPUNIT_ASSERT_EQUAL(expected, actual);
}


/**
 * Test encoding a string with the MD5 digest algorithm.
 */
void
CipherTest::testMd5_7(void)
{
    const std::string expected("57edf4a22be3c955ac49da2e2107b67a");

    std::string s;
    for (int i = 0; i < 8; ++i) {
        s += "1234567890";
    }
    std::string actual(Cipher::instance().md5(s));

    CPPUNIT_ASSERT_EQUAL(expected, actual);
}
