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

const char sqlAccountTable[] =
"create table tmw_accounts ("
    //username
    "user varchar(32) unique primary key not null,"
    //password hash
    "password varchar(32) not null,"
    //email address
    "email varchar(128) not null"
    //account type (should we have "admin" table? or should we have account
    //type here?)
//    "type int not null"
")";

const char sqlCharacterTable[] =
"create table tmw_characters ("
    //character name
    "name varchar(32) unique primary key not null,"
    //user name
    "user varchar(32) not null,"
    //player information
    "gender int not null,"
    "level int not null,"
    "money int not null,"
    //coordinates
    "x int not null,"
    "y int not null,"
    //map name
    "map text not null,"
    //statistics
    "strength int not null,"
    "agility int not null,"
    "vitality int not null,"
    "intelligence int not null,"
    "dexterity int not null,"
    "luck int not null,"
    //player equipment
//    "inventory blob not null," // TODO: blob bad
//    "equipment blob not null," // TODO: blob bad
    //table relationship
    "foreign key(user) references tmw_accounts(user)"
")";

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

    // Clean up managed accounts & characters
    for (unsigned int i = 0; i < accounts.size(); i++)
        delete accounts[i];
    for (unsigned int i = 0; i < characters.size(); i++)
        delete characters[i];

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
    if (sqlite.DirectStatement(sqlAccountTable)) {
        std::cout << "Database: table tmw_accounts created" << std::endl;
    }
    else {
        std::cout << "Database: table tmw_accounts exists" << std::endl;
    }

    // Characters table
    if (sqlite.DirectStatement(sqlCharacterTable)) {
        std::cout << "Database: table tmw_characters created" << std::endl;
    }
    else {
        std::cout << "Database: table tmw_characters exists" << std::endl;
    }

    //populate table for the hell of it ;)
    sqlite.DirectStatement("insert into tmw_accounts values ('nym', 'tHiSiSHaShEd', 'nym@test')");
    sqlite.DirectStatement("insert into tmw_accounts values ('Bjorn', 'tHiSiSHaShEd', 'bjorn@test')");
    sqlite.DirectStatement("insert into tmw_accounts values ('Usiu', 'tHiSiSHaShEd', 'usiu@test')");
    sqlite.DirectStatement("insert into tmw_accounts values ('ElvenProgrammer', 'tHiSiSHaShEd', 'elven@test')");
    sqlite.DirectStatement("insert into tmw_characters values ('Nym the Great', 'nym', 0, 99, 1000000, 0, 0, 'main.map', 1, 2, 3, 4, 5, 6)");

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

Account* Storage::getAccount(const std::string &username)
{
#ifdef SQLITE_SUPPORT
    //give caller a initialized AccountData structure
    SQLiteWrapper::ResultTable r;

    //make sure account isn't loaded already
    for (unsigned int i = 0; i < accounts.size(); i++)
        if (accounts[i]->getName() == username)
            return accounts[i];

    std::string selectStatement = "select * from tmw_accounts where user = \""
                                  + username + "\"";
    if (!sqlite.SelectStmt(selectStatement, r))
    {
        return NULL;
    }

    Account *acc = new Account;
    acc->setName(r.records_[0].fields_[0]);
    acc->setPassword(r.records_[0].fields_[1]);
    acc->setEmail(r.records_[0].fields_[2]);
    accounts.push_back(acc);

    return acc;
#else
    return NULL;
#endif
}

Being* Storage::getCharacter(const std::string &username)
{
#ifdef SQLITE_SUPPORT
    //give caller a initialized AccountData structure
    SQLiteWrapper::ResultTable r;

    std::string selectStatement = "select * from tmw_characters where user = \""
                                  + username + "\"";
    if (!sqlite.SelectStmt(selectStatement, r))
        return NULL;
    if (r.records_.size() == 0)
        return NULL;

    //make sure account isn't loaded already
    for (unsigned int i = 0; i < characters.size(); i++)
        if (characters[i]->getName() == r.records_[0].fields_[0])
            return characters[i];

    Being *acc = new Being(r.records_[0].fields_[0], 1, 1, 1, 1, 1, 1, 1, 1);
    characters.push_back(acc);

    return acc;
#else
    return NULL;
#endif
}

