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


#include "testaccount.h"


// register the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(AccountTest);


using namespace tmwserv;


/**
 * Set up fixtures.
 */
void
AccountTest::setUp(void)
{
    //RawStatistics stats;
    //stats.strength = 1;
    //stats.agility = 1;
    //stats.vitality = 1;
    //stats.intelligence = 1;
    //stats.dexterity = 1;
    //stats.luck = 1;
    const RawStatistics stats = {1, 1, 1, 1, 1, 1};

    Being* sam = new Being("sam", GENDER_MALE, 0, 0, stats);
    Being* merry = new Being("merry", GENDER_MALE, 0, 0, stats);
    Being* pippin = new Being("pippin", GENDER_MALE, 0, 0, stats);
    mCharacters.push_back(sam);
    mCharacters.push_back(merry);
    mCharacters.push_back(pippin);

    mAccount = new Account("frodo",
                           "baggins",
                           "frodo@theshire.com",
                           mCharacters);
}


/**
 * Tear down fixtures.
 */
void
AccountTest::tearDown(void)
{
    for (Beings::iterator it = mCharacters.begin();
         it != mCharacters.end();
         ++it)
    {
        delete (*it);
    }

    delete mAccount;
    mAccount = 0;
}


/**
 * Test creating an Account passing the initial account info
 * to the constructor.
 */
void
AccountTest::testCreate1(void)
{
    const std::string name("frodo");
    const std::string password("baggins");
    const std::string email("frodo@theshire.com");

    Account account(name, password, email);

    CPPUNIT_ASSERT_EQUAL(name, account.getName());
    CPPUNIT_ASSERT_EQUAL(password, account.getPassword());
    CPPUNIT_ASSERT_EQUAL(email, account.getEmail());
    CPPUNIT_ASSERT_EQUAL((size_t) 0, account.getCharacters().size());
}


/**
 * Test creating an Account passing the initial account info
 * to the constructor.
 */
void
AccountTest::testCreate2(void)
{
    const std::string name("frodo");
    const std::string password("baggins");
    const std::string email("frodo@theshire.com");

    Account account(name, password, email, mCharacters);

    CPPUNIT_ASSERT_EQUAL(name, account.getName());
    CPPUNIT_ASSERT_EQUAL(password, account.getPassword());
    CPPUNIT_ASSERT_EQUAL(email, account.getEmail());

    CPPUNIT_ASSERT_EQUAL(mCharacters.size(),
                         mAccount->getCharacters().size());

    Beings& characters = account.getCharacters();

    for (size_t i = 0; i < mCharacters.size(); ++i) {
        CPPUNIT_ASSERT_EQUAL(characters[i]->getName(),
                             mCharacters[i]->getName());
    }
}


/**
 * Test adding a new character.
 */
void
AccountTest::testAddCharacter1(void)
{
    RawStatistics stats;
    stats.strength = 1;
    stats.agility = 1;
    stats.vitality = 1;
    stats.intelligence = 1;
    stats.dexterity = 1;
    stats.luck = 1;

    Being* bilbo = new Being("bilbo", GENDER_MALE, 0, 0, stats);

    mAccount->addCharacter(bilbo);

    CPPUNIT_ASSERT_EQUAL((size_t) 4, mAccount->getCharacters().size());

    std::vector<std::string> names;
    names.push_back("sam");
    names.push_back("merry");
    names.push_back("pippin");
    names.push_back("bilbo");

    for (size_t i = 0; i < mCharacters.size(); ++i) {
        CPPUNIT_ASSERT_EQUAL(names[i], mCharacters[i]->getName());
    }

    delete bilbo;
}


/**
 * Test passing a NULL pointer to addCharacter().
 */
void
AccountTest::testAddCharacter2(void)
{
    mAccount->addCharacter(NULL);

    CPPUNIT_ASSERT_EQUAL((size_t) 3, mAccount->getCharacters().size());

    std::vector<std::string> names;
    names.push_back("sam");
    names.push_back("merry");
    names.push_back("pippin");

    for (size_t i = 0; i < mCharacters.size(); ++i) {
        CPPUNIT_ASSERT_EQUAL(names[i], mCharacters[i]->getName());
    }
}


/**
 * Test the accessor getCharacter() with a valid name.
 */
void
AccountTest::testGetCharacter1(void)
{
    const std::string name("merry");

    Being* merry = mAccount->getCharacter(name);

    CPPUNIT_ASSERT(merry != 0);
    CPPUNIT_ASSERT_EQUAL(name, merry->getName());
}


/**
 * Test the accessor getCharacter() with an unexisting name.
 */
void
AccountTest::testGetCharacter2(void)
{
    Being* nobody = mAccount->getCharacter("johndoe");

    CPPUNIT_ASSERT(nobody == 0);
}
