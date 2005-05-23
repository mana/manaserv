/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */
#include "storage.h"
#include <iostream>

Storage::Storage()
{
#ifdef SQLITE_SUPPORT
    // Open database
    if (sqlite.Open("tmw.db")) {
        std::cout << "Database: tmw.db created or opened" << std::endl;
    }
    else {
        std::cout << "Database: couldn't open tmw.db" << std::endl;
    }
#endif
#ifdef PGSQL_SUPPORT
    std::cout << "Datbase: Postgresql currently not supported" << std::endl;
    exit(1);
#endif

    //Create tables
    create_tables_if_necessary();
}

Storage::~Storage()
{
#ifdef SQLITE_SUPPORT
    // Close database
    if (sqlite.Close()) {
        std::cout << "Database: tmw.db closed" << std::endl;
    }
    else {
        std::cout << "Database: couldn't close tmw.db" << std::endl;
    }
#endif
}

/**
 * Create tables if necessary
 */
void Storage::create_tables_if_necessary()
{
#ifdef SQLITE_SUPPORT
    SQLiteWrapper::ResultTable r;

    if (!sqlite.SelectStmt("select count(*) from sqlite_master where tbl_name='topics' and type='table'", r)) {
        std::cout << "Error with select count(*) [create_tables_if_necessary]" << sqlite.LastError().c_str() << std::endl;
    }
    
    if (r.records_[0].fields_[0] != "0")
      return;
    
    sqlite.Begin();
    if (sqlite.DirectStatement("create table tmw_accounts(user TEXT, password TEXT, email TEXT)")) {
        std::cout << "Database: table tmw_accounts created" << std::endl;
    }
    else {
        std::cout << "Database: table exist" << std::endl;
    }
    sqlite.Commit();
#endif
}

