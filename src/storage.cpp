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
#include <sstream>

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
    // Accounts table
    if (sqlite.DirectStatement("create table tmw_accounts ("
                               //username
                               "  user varchar(32) unique primary key not null,"
                               //password hash
                               "  password varchar(32) not null,"
                               //email address
                               "  email varchar(128) not null,"
                               //account type
                               "  type int not null"
                               ")")) {
        std::cout << "Database: table tmw_accounts created" << std::endl;
    }
    else {
        std::cout << "Database: table tmw_accounts exists" << std::endl;
    }

    // Characters table
    if (sqlite.DirectStatement("create table tmw_characters ("
                               //character name
                               "  name varchar(32) unique primary key not null,"
                               //user name
                               "  user varchar(32) not null,"
                               //player information
                               "  gender int not null,"
                               "  level int not null,"
                               "  money int not null,"
                               //coordinates
                               "  x int not null,"
                               "  y int not null,"
                               //map name
                               "  map text not null,"
                               //statistics
                               "  strength int not null,"
                               "  agility int not null,"
                               "  vitality int not null,"
                               "  intelligence int not null,"
                               "  dexterity int not null,"
                               "  luck int not null,"
                               //player equipment
                               "  inventory blob not null," // TODO: blob bad
                               "  equipment blob not null," // TODO: blob bad
                               //table relationship
                               "  foreign key(user) references tmw_accounts(user)"
                               ")")) {
        std::cout << "Database: table tmw_characters created" << std::endl;
    }
    else {
        std::cout << "Database: table tmw_characters exists" << std::endl;
    }

    //populate table for the hell of it ;)
    sqlite.DirectStatement("insert into tmw_accounts values ('nym', 'tHiSiSHaShEd', 'nym@test', 0)");
    sqlite.DirectStatement("insert into tmw_accounts values ('Bjorn', 'tHiSiSHaShEd', 'bjorn@test', 0)");
    sqlite.DirectStatement("insert into tmw_accounts values ('Usiu', 'tHiSiSHaShEd', 'usiu@test', 0)");
    sqlite.DirectStatement("insert into tmw_accounts values ('ElvenProgrammer', 'tHiSiSHaShEd', 'elven@test', 0)");

    sqlite.Commit();
#endif
}

void Storage::save()
{
#ifdef SQLITE_SUPPORT
    sqlite.Commit();
#endif
}

unsigned int Storage::accountCount()
{
#ifdef SQLITE_SUPPORT
    SQLiteWrapper::ResultTable r;
    std::stringstream s;
    unsigned int v;
    sqlite.SelectStmt("select count(*) from tmw_accounts", r);
    s << r.records_[0].fields_[0];
    s >> v;
    return v;
#else
    return 0;
#endif
}

/*
Account& getAccount(const std::string &username)
{
    //give caller a initialized AccountData structure
}
*/

