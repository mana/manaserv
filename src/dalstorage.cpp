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

#include "dalstorage.h"
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
  ");";

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
  ");";

/*
 * All items in the game world are stored in this table.
 */
const char sqlItemTable[] =
  "create table tmw_items ("
    "id        int          unique primary key not null,"
    "amount    int          not null,"  // Items of same kind can stack
    "type      int          not null,"  // Type as defined in item database
    "state     text"                    // Optional item state saved by script
  ");";

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
  ");";

/*
 * Character Inventory
 */
const char sqlInventoryTable[] =
  "create table tmw_inventory ("
    "id        int          primary key not null," // Item ID
    "owner_id  int          not null," // Owner character ID
    "foreign key(id) references tmw_items(id),"
    "foreign key(owner_id) references tmw_characters(id)"
  ");";


DALStorage::DALStorage()
{
    db = tmw::dal::DataProviderFactory::createDataProvider();
    std::string dbName = "tmw";
    // Try connection to database
    try {
	db->connect("tmw.db", "", "");
    } catch (tmw::dal::DbConnectionFailure f) {
	std::cout << "Database connection failed." << std::endl;
	// Try creating database
	try {
	    db->createDb("tmw.db");
	    db->connect("tmw.db", "", "");
	    // Create tables
	    db->execSql(sqlAccountTable);
	    db->execSql(sqlCharacterTable);
	    db->execSql(sqlItemTable);
	    db->execSql(sqlWorldItemTable);
	    db->execSql(sqlInventoryTable);
	    // Example data :)
	    db->execSql("insert into tmw_accounts values (0, 'nym', 'tHiSiSHaShEd', 'nym@test', 1, 0);");
	    db->execSql("insert into tmw_accounts values (1, 'Bjorn', 'tHiSiSHaShEd', 'bjorn@test', 1, 0);");
	    db->execSql("insert into tmw_accounts values (2, 'Usiu', 'tHiSiSHaShEd', 'usiu@test', 1, 0);");
	    db->execSql("insert into tmw_accounts values (3, 'ElvenProgrammer', 'tHiSiSHaShEd', 'elven@test', 1, 0);");
	    db->execSql("insert into tmw_characters values (0, 0, 'Nym the Great', 0, 99, 1000000, 0, 0, 'main.map', 1, 2, 3, 4, 5, 6);");
	    //
	} catch (tmw::dal::DbCreationFailure f) {
	    std::cout << "Database creation failed." << std::endl;
	} catch (tmw::dal::DbSqlQueryExecFailure f) {
	    std::cout << "Database table creation failed" << std::endl;
	}
    }
}

DALStorage::~DALStorage()
{
    db->disconnect();
    delete db;

    // clean up loaded accounts
    for (unsigned int i = 0; i < accounts.size(); i++)
	delete accounts[i];
}

void DALStorage::flush()
{
    // this isn't required for DAL
}

unsigned int DALStorage::getAccountCount()
{
    try {
	const tmw::dal::RecordSet &r = db->execSql("select count(*) from tmw_accounts;");
	std::stringstream s;
	unsigned int tmp;

	s << r(0, 0);
	s >> tmp;
	return tmp;
    } catch (tmw::dal::DbSqlQueryExecFailure f) {
	std::cout << "Get accounts count failed :'(" << std::endl;
    }
    return 0;
}

Account *DALStorage::getAccount(const std::string &username)
{
    for (unsigned int i = 0; i < accounts.size(); i++)
	if (accounts[i]->getName() == username)
	    return accounts[i];

    std::string selectStatement = "select * from tmw_accounts where username = '"
	+ username + "';";

    try {
	const tmw::dal::RecordSet &r = db->execSql(selectStatement);

	// Create account
	Account *acc = new Account();
	acc->setName(r(0, 1));
	acc->setPassword(r(0, 2));
	acc->setEmail(r(0, 4));
	accounts.push_back(acc);
	
	// Load character associated with the account
	std::string user_id = r(0, 0);
	selectStatement = "select * from tmw_characters where id = '" + user_id + "';";
	try {
	    const tmw::dal::RecordSet &r2 = db->execSql(selectStatement);

	    std::vector<Being*> beings;
	    
	    for (unsigned int i = 0; i < r2.rows(); i++)
	    {
		Being *being = new Being(
					 r2(i, 2), 1, 1, 1, 1, 1, 1, 1, 1);
		characters.push_back(being);
		beings.push_back(being);
	    }
	    
	    acc->setCharacters(beings);
	    //
	} catch (tmw::dal::DbSqlQueryExecFailure f) {
	    return NULL; // TODO: Throw exception here
	}
	
	return acc;
	//
    } catch (tmw::dal::DbSqlQueryExecFailure f) {
	return NULL; // TODO: Throw exception here
    }
}
