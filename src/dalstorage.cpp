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


#include <sstream>

#include "dalstorage.h"


namespace
{


/**
 * Functor used for the search of an Account by name.
 */
struct account_name_equals_to
    : public std::binary_function<Account*, std::string, bool>
{
    bool
    operator()(Account* account,
               const std::string& name) const
    {
        return account->getName() == name;
    }
};


/**
 * Functor to convert a string into another type using
 * std::istringstream.operator>>().
 */
template <typename T>
struct string_to: public std::unary_function<std::string, T>
{
    T
    operator()(const std::string& s) const
    {
        std::istringstream is(s);
        T value;
        is >> value;

        return value;
    }
};


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


} // anonymous namespace


namespace tmwserv
{


/**
 * Constructor.
 */
DALStorage::DALStorage()
        : mDb(dal::DataProviderFactory::createDataProvider())
{
    // the connection to the database will be made on the first request
    // to the database.
}


/**
 * Destructor.
 */
DALStorage::~DALStorage()
    throw()
{
    mDb->disconnect();

    // clean up loaded accounts.
    for (Accounts::iterator it = mAccounts.begin();
         it != mAccounts.end();
         ++it)
    {
        delete (*it);
    }

    // clean up loaded characters.
    for (Beings::iterator it = mCharacters.begin();
         it != mCharacters.end();
         ++it)
    {
        delete (*it);
    }
}


/**
 * Save changes to the database permanently.
 */
void
DALStorage::flush(void)
{
    // this feature is not currently provided by DAL.
}


/**
 * Get the number of Accounts saved in database.
 */
unsigned int
DALStorage::getAccountCount(void)
{
    // connect to the database (if not connected yet).
    connect();

    using namespace dal;

    try {
        // query the database.
        const std::string sql = "select count(*) from tmw_accounts;";
        const RecordSet& rs = mDb->execSql(sql);

        // convert the result into a number.
        std::istringstream s(rs(0, 0));
        unsigned int value;
        s >> value;

        return value;
    } catch (const DbSqlQueryExecFailure& f) {
        std::cout << "Get accounts count failed :'(" << std::endl;
    }
}


/**
 * Get an account by user name.
 */
Account*
DALStorage::getAccount(const std::string& userName)
{
    // connect to the database (if not connected yet).
    connect();

    // look for the account in the list first.
    Accounts::iterator it =
        std::find_if(
            mAccounts.begin(),
            mAccounts.end(),
            std::bind2nd(account_name_equals_to(), userName)
        );

    if (it != mAccounts.end()) {
        return (*it);
    }

    using namespace dal;

    // the account was not in the list, look for it in the database.
    try {
        std::string sql("select * from tmw_accounts where username = '");
        sql += userName;
        sql += "';";
        const RecordSet& accountInfo = mDb->execSql(sql);

        // if the account is not even in the database then
        // we have no choice but to return nothing.
        if (accountInfo.isEmpty()) {
            return NULL;
        }

        // create an Account instance
        // and initialize it with information about the user.
        Account* account = new Account();
        account->setName(accountInfo(0, 1));
        account->setPassword(accountInfo(0, 2));
        account->setEmail(accountInfo(0, 3));

        // add the new Account to the list.
        mAccounts.push_back(account);

        // load the characters associated with the account.
        sql = "select * from tmw_characters where id = '";
        sql += accountInfo(0, 0);
        sql += "';";
        const RecordSet& charInfo = mDb->execSql(sql);

        if (!charInfo.isEmpty()) {
            Beings beings;

            // specialize the string_to functor to convert
            // a string to an unsigned int.
            string_to<unsigned int> toUint;

            for (unsigned int i = 0; i < charInfo.rows(); ++i) {
                Being* being =
                    new Being(charInfo(i, 2),          // name
                              toUint(charInfo(i, 3)),  // gender
                              toUint(charInfo(i, 4)),  // level
                              toUint(charInfo(i, 5)),  // money
                              toUint(charInfo(i, 9)),  // strength
                              toUint(charInfo(i, 10)), // agility
                              toUint(charInfo(i, 11)), // vitality
                              toUint(charInfo(i, 13)), // dexterity
                              toUint(charInfo(i, 14))  // luck
                             );

                mCharacters.push_back(being);
                beings.push_back(being);
            }

            account->setCharacters(beings);
        }

        return account;
    }
    catch (const DbSqlQueryExecFailure& e) {
        return NULL; // TODO: Throw exception here
    }
}


/**
 * Connect to the database and initialize it if necessary.
 */
void
DALStorage::connect(void)
{
    // do nothing if already connected.
    if (mDb->isConnected()) {
        return;
    }

    using namespace dal;

    try {
        // open a connection to the database.
        // TODO: get the database name, the user name and the user password
        // from a configuration manager.
        mDb->connect("tmw", "", "");

        bool doInitDb = true;

        // TODO: check the existence of the tables first and
        // create only those that are missing.

        if (doInitDb) {
            // create the tables.
            mDb->execSql(sqlAccountTable);
            mDb->execSql(sqlCharacterTable);
            mDb->execSql(sqlItemTable);
            mDb->execSql(sqlWorldItemTable);
            mDb->execSql(sqlInventoryTable);

            // Example data :)
            mDb->execSql("insert into tmw_accounts values (0, 'nym', 'tHiSiSHaShEd', 'nym@test', 1, 0);");
            mDb->execSql("insert into tmw_accounts values (1, 'Bjorn', 'tHiSiSHaShEd', 'bjorn@test', 1, 0);");
            mDb->execSql("insert into tmw_accounts values (2, 'Usiu', 'tHiSiSHaShEd', 'usiu@test', 1, 0);");
            mDb->execSql("insert into tmw_accounts values (3, 'ElvenProgrammer', 'tHiSiSHaShEd', 'elven@test', 1, 0);");
            mDb->execSql("insert into tmw_characters values (0, 0, 'Nym the Great', 0, 99, 1000000, 0, 0, 'main.map', 1, 2, 3, 4, 5, 6);");
        }
    }
    catch (const DbConnectionFailure& e) {
        std::cout << "unable to connect to the database: "
                  << e.what() << std::endl;
    }
    catch (const DbSqlQueryExecFailure& e) {
        std::cout << e.what() << std::endl;
    }
}


} // namespace tmwserv
