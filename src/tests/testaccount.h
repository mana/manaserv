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


#ifndef _TMWSERV_TEST_ACCOUNT_H_
#define _TMWSERV_TEST_ACCOUNT_H_


#include <cppunit/extensions/HelperMacros.h>

#include "../account.h"


/**
 * Unit test for the Account class.
 */
class AccountTest: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(AccountTest);

    // add tests to the test suite.
    CPPUNIT_TEST(testCreate1);
    CPPUNIT_TEST(testCreate2);
    CPPUNIT_TEST(testAddCharacter1);
    CPPUNIT_TEST(testAddCharacter2);
    CPPUNIT_TEST(testGetCharacter1);
    CPPUNIT_TEST(testGetCharacter2);

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
         * Test creating an Account passing the initial account info
         * to the constructor.
         */
        void
        testCreate1(void);


        /**
         * Test creating an Account passing the initial account info
         * to the constructor.
         */
        void
        testCreate2(void);


        /**
         * Test adding a new character.
         */
        void
        testAddCharacter1(void);


        /**
         * Test passing a NULL pointer to addCharacter().
         */
        void
        testAddCharacter2(void);


        /**
         * Test the accessor getCharacter() with a valid name.
         */
        void
        testGetCharacter1(void);


        /**
         * Test the accessor getCharacter() with an unexisting name.
         */
        void
        testGetCharacter2(void);


    private:
        tmwserv::Account* mAccount;  /**< the default account */
        tmwserv::Beings mCharacters; /**< a list of beings */
};


#endif // _TMWSERV_TEST_ACCOUNT_H_
