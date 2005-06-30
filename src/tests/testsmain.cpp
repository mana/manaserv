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


#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>

#include <physfs.h>

#include "../utils/logger.h"


/**
 * Notes:
 *     - if the unit test application is linked to libsqlite3, there will
 *       be a memory leak (8 bytes) while no leaks are detected when linked
 *       to libmysqlclient. the leak was detected using Valgrind, an
 *       excellent memory debugger.
 *
 * TODO: check memory leak when linked to libpq (PostgreSQL).
 */


int main(int argc, char* argv[])
{
    // initialize the PhysicsFS library.
    PHYSFS_init(argv[0]);
    PHYSFS_addToSearchPath(".", 1);
    PHYSFS_setWriteDir(".");

    // initialize the logger.
    tmwserv::utils::Logger::instance().setTimestamp(false);

    using namespace CppUnit;

    // get the top level suite from the registry.
    Test* suite = TestFactoryRegistry::getRegistry().makeTest();

    // add the test to the list of test to run.
    TextTestRunner runner;
    runner.addTest(suite);

    // run the tests.
    bool wasSuccessful = runner.run();

    // denitialize the PhysicsFS library.
    PHYSFS_deinit();

    // return error code 1 if the one of test failed.
    return wasSuccessful ? 0 : 1;
}
