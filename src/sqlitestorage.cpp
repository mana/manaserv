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

#include "sqlitestorage.h"

#include <iostream>
#include <sstream>

/* Values for user level could be:
 *  0: Normal user
 *  1: Moderator (has medium level rights)
 *  2: Administrator (can do basically anything)
 */
const char sqlAccountTable[] =
  "create table tmw_accounts ("
    "id        int          unique primary key not null,"
    "username  varchar(32)  not null,"
    "password  varchar(32)  not null,"
    "email     varchar(128) not null,"
    "level     int          not null,"  // User level (normal, admin, etc.)
    "banned    int          not null"   // The UNIX time of unban (0 default)
  ")";

/* Note: The stats will need to be thought over, as we'll be implementing a
 *  much more elaborate skill based system. We should probably have a separate
 *  table for storing the skill levels.
 *
 * Gender is 0 for male, 1 for female.
 */
const char sqlCharacterTable[] =
  "create table tmw_characters ("
    "id        int          unique primary key not null,"
    "user_id   int          not null,"
    "name      varchar(32)  not null,"
    "gender    int          not null,"  // Player information
    "level     int          not null,"
    "money     int          not null,"
    "x         int          not null,"  // Location
    "y         int          not null,"
    "map       text         not null,"
    "str       int          not null,"  // Stats
    "agi       int          not null,"
    "vit       int          not null,"
    "int       int          not null,"
    "dex       int          not null,"
    "luck      int          not null,"
    "foreign key(user_id) references tmw_accounts(id)"
  ")";

/*
 * All items in the game world are stored in this table.
 */
const char sqlItemTable[] =
  "create table tmw_items ("
    "id        int          unique primary key not null,"
    "amount    int          not null,"  // Items of same kind can stack
    "type      int          not null,"  // Type as defined in item database
    "state     text"                    // Optional item state saved by script
  ")";

/*
 * Items on the ground in the game world.
 */
const char sqlWorldItemTable[] =
  "create table tmw_world_items ("
    "id        int          not null,"
    "map       text,"
    "x         int          not null,"  // Location of item on map
    "y         int          not null,"
    "deathtime int          not null,"  // Time to die (UNIX time)
    "primary key(id, map),"
    "foreign key(id) references tmw_items(id)"
  ")";

/*
 * Character Inventory
 */
const char sqlInventoryTable[] =
  "create table tmw_inventory ("
    "id        int          primary key not null," // Item ID
    "owner_id  int          not null," // Owner character ID
    "foreign key(id) references tmw_items(id),"
    "foreign key(owner_id) references tmw_characters(id)"
  ")";


SQLiteStorage::SQLiteStorage()
{
    // Open database
    if (sqlite.Open("tmw.db")) {
        std::cout << "Database: tmw.db created or opened" << std::endl;
    }
    else {
        std::cout << "Database: couldn't open tmw.db" << std::endl;
    }

    // Create tables
    createTablesIfNecessary();
}

SQLiteStorage::~SQLiteStorage()
{
    // Make sure any changes have been written
    flush();

    // Close database
    if (sqlite.Close())
    {
        std::cout << "Database: tmw.db closed" << std::endl;
    }
    else
    {
        std::cout << "Database: couldn't close tmw.db" << std::endl;
    }

    // Clean up managed accounts & characters
    for (unsigned int i = 0; i < accounts.size(); i++)
    {
        delete accounts[i];
    }
    for (unsigned int i = 0; i < characters.size(); i++)
    {
        delete characters[i];
    }
}

void SQLiteStorage::createTablesIfNecessary()
{
    SQLiteWrapper::ResultTable r;

    if (!sqlite.SelectStmt("select count(*) from sqlite_master where tbl_name='topics' and type='table'", r))
    {
        std::cout << "Error with select count(*) [createTablesIfNecessary]" << sqlite.LastError().c_str() << std::endl;
    }

    if (r.records_[0].fields_[0] != "0") {
        return;
    }

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

    // Items table
    if (sqlite.DirectStatement(sqlItemTable)) {
        std::cout << "Database: table tmw_items created" << std::endl;
    }
    else {
        std::cout << "Database: table tmw_items exists" << std::endl;
    }

    // World Items table
    if (sqlite.DirectStatement(sqlWorldItemTable)) {
        std::cout << "Database: table tmw_world_items created" << std::endl;
    }
    else {
        std::cout << "Database: table tmw_world_items exists" << std::endl;
    }

    // Character Inventory
    if (sqlite.DirectStatement(sqlInventoryTable)) {
        std::cout << "Database: table tmw_inventory created" << std::endl;
    }
    else {
        std::cout << "Database: table tmw_inventory exists" << std::endl;
    }

    // Populate table for the hell of it ;)
    sqlite.DirectStatement("insert into tmw_accounts values (0, 'nym', 'tHiSiSHaShEd', 'nym@test', 1, 0)");
    sqlite.DirectStatement("insert into tmw_accounts values (1, 'Bjorn', 'tHiSiSHaShEd', 'bjorn@test', 1, 0)");
    sqlite.DirectStatement("insert into tmw_accounts values (2, 'Usiu', 'tHiSiSHaShEd', 'usiu@test', 1, 0)");
    sqlite.DirectStatement("insert into tmw_accounts values (3, 'ElvenProgrammer', 'tHiSiSHaShEd', 'elven@test', 1, 0)");
    sqlite.DirectStatement("insert into tmw_characters values (0, 0, 'Nym the Great', 0, 99, 1000000, 0, 0, 'main.map', 1, 2, 3, 4, 5, 6)");

    flush();
}

void SQLiteStorage::flush()
{
    sqlite.Commit();
}

unsigned int SQLiteStorage::getAccountCount()
{
    SQLiteWrapper::ResultTable r;
    std::stringstream s;
    unsigned int v;
    sqlite.SelectStmt("select count(*) from tmw_accounts", r);
    s << r.records_[0].fields_[0];
    s >> v;
    return v;
}

Account* SQLiteStorage::getAccount(const std::string &username)
{
    // Make sure account isn't loaded already
    for (unsigned int i = 0; i < accounts.size(); i++)
    {
        if (accounts[i]->getName() == username)
        {
            return accounts[i];
        }
    }

    // Give caller a initialized AccountData structure
    SQLiteWrapper::ResultTable r;

    std::string selectStatement =
        "select * from tmw_accounts where username = \"" + username + "\"";

    if (!sqlite.SelectStmt(selectStatement, r))
    {
        // Throwing an exception here could be good
        return NULL;
    }

    Account *acc = new Account();
    acc->setName(r.records_[0].fields_[1]);
    acc->setPassword(r.records_[0].fields_[2]);
    acc->setEmail(r.records_[0].fields_[3]);
    accounts.push_back(acc);

    std::string user_id = r.records_[0].fields_[0];

    // Load the characters associated with the account
    selectStatement =
        "select * from tmw_characters where id = \"" + user_id + "\"";

    if (!sqlite.SelectStmt(selectStatement, r))
    {
        // Throwing exception here could be good
        return NULL;
    }

    std::vector<Being*> beings;

    for (unsigned int i = 0; i < r.records_.size(); i++)
    {
        Being *being = new Being(
                r.records_[i].fields_[2], 1, 1, 1, 1, 1, 1, 1, 1);
        characters.push_back(being);
        beings.push_back(being);
    }

    acc->setCharacters(beings);

    return acc;
}
