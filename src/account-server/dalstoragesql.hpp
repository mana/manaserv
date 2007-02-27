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


#ifndef _TMWSERV_DALSTORAGE_SQL_H_
#define _TMWSERV_DALSTORAGE_SQL_H_


#if !defined (MYSQL_SUPPORT) && !defined (SQLITE_SUPPORT) && \
    !defined (POSTGRESQL_SUPPORT)

#error "(dalstorage.h) no database backend defined"
#endif


#include <string>

// TODO: Fix problem with PostgreSQL null primary key's.

/**
 * MySQL specificities:
 *     - TINYINT is an integer (1 byte) type defined as an extension to
 *       the SQL standard.
 *     - all integer types can have an optional (non-standard) attribute
 *       UNSIGNED (http://dev.mysql.com/doc/mysql/en/numeric-types.html)
 *
 * SQLite3 specificities:
 *     - any column (but only one for each table) with the exact type of
 *       'INTEGER PRIMARY KEY' is taken as auto-increment.
 *     - the supported data types are: NULL, INTEGER, REAL, TEXT and BLOB
 *       (http://www.sqlite.org/datatype3.html)
 *     - the size of TEXT cannot be set, it is just ignored by the engine.
 *     - IMPORTANT: foreign key constraints are not yet supported
 *       (http://www.sqlite.org/omitted.html). Included in case of future
 *       support.
 *
 * Notes:
 *     - the SQL queries will take advantage of the most appropriate data
 *       types supported by a particular database engine in order to
 *       optimize the server database size.
 */


namespace {


/**
 * TABLE: tmw_accounts.
 *
 * Notes:
 *     - the user levels are:
 *           0: normal user
 *           1: moderator (has medium level rights)
 *           2: administrator (i am god :))
 *     - the 'banned' field contains the UNIX time of unban (default = 0)
 */
const std::string ACCOUNTS_TBL_NAME("tmw_accounts");
const std::string SQL_ACCOUNTS_TABLE(
    "CREATE TABLE tmw_accounts ("
#if defined (MYSQL_SUPPORT)
        "id       INTEGER     PRIMARY KEY AUTO_INCREMENT,"
        "username VARCHAR(32) NOT NULL UNIQUE,"
        "password VARCHAR(32) NOT NULL,"
        "email    VARCHAR(64) NOT NULL,"
        "level    TINYINT     UNSIGNED NOT NULL,"
        "banned   TINYINT     UNSIGNED NOT NULL,"
        "INDEX (id)"
#elif defined (SQLITE_SUPPORT)
        "id       INTEGER     PRIMARY KEY,"
        "username TEXT        NOT NULL UNIQUE,"
        "password TEXT        NOT NULL,"
        "email    TEXT        NOT NULL,"
        "level    INTEGER     NOT NULL,"
        "banned   INTEGER     NOT NULL"
#elif defined (POSTGRESQL_SUPPORT)
        "id       SERIAL      PRIMARY KEY,"
        "username TEXT        NOT NULL UNIQUE,"
        "password TEXT        NOT NULL,"
        "email    TEXT        NOT NULL,"
        "level    INTEGER     NOT NULL,"
        "banned   INTEGER     NOT NULL"
#endif
    ");"
);


/**
 * TABLE: tmw_characters.
 *
 * Notes:
 *     - the stats will need to be thought over, as we'll be implementing a
 *       much more elaborate skill based system; we should probably have a
 *       separate table for storing the skill levels.
 *     - gender is 0 for male, 1 for female.
 */
const std::string CHARACTERS_TBL_NAME("tmw_characters");
const std::string SQL_CHARACTERS_TABLE(
    "CREATE TABLE tmw_characters ("
#if defined (MYSQL_SUPPORT)
        "id      INTEGER     PRIMARY KEY AUTO_INCREMENT,"
        "user_id INTEGER     UNSIGNED NOT NULL,"
        "name    VARCHAR(32) NOT NULL UNIQUE,"
        // general information about the character
        "gender      TINYINT     UNSIGNED NOT NULL,"
        "hair_style  TINYINT     UNSIGNED NOT NULL,"
        "hair_color  TINYINT     UNSIGNED NOT NULL,"
        "level   TINYINT     UNSIGNED NOT NULL,"
        "money   INTEGER     UNSIGNED NOT NULL,"
        // location on the map
        "x       SMALLINT    UNSIGNED NOT NULL,"
        "y       SMALLINT    UNSIGNED NOT NULL,"
        "map_id  TINYINT     NOT NULL,"
        // stats
        "str     SMALLINT    UNSIGNED NOT NULL,"
        "agi     SMALLINT    UNSIGNED NOT NULL,"
        "vit     SMALLINT    UNSIGNED NOT NULL,"
        // note: int must be backquoted as it's a MySQL keyword
        "`int`   SMALLINT    UNSIGNED NOT NULL,"
        "dex     SMALLINT    UNSIGNED NOT NULL,"
        "luck    SMALLINT    UNSIGNED NOT NULL,"
        "FOREIGN KEY (user_id) REFERENCES tmw_accounts(id),"
        "FOREIGN KEY (map_id)  REFERENCES tmw_maps(id),"
        "INDEX (id)"
#elif defined (SQLITE_SUPPORT)
        "id      INTEGER     PRIMARY KEY,"
        "user_id INTEGER     NOT NULL,"
        "name    TEXT        NOT NULL UNIQUE,"
        // general information about the character
        "gender      INTEGER     NOT NULL,"
        "hair_style  INTEGER     NOT NULL,"
        "hair_color  INTEGER     NOT NULL,"
        "level   INTEGER     NOT NULL,"
        "money   INTEGER     NOT NULL,"
        // location on the map
        "x       INTEGER     NOT NULL,"
        "y       INTEGER     NOT NULL,"
        "map_id  INTEGER     NOT NULL,"
        // stats
        "str     INTEGER     NOT NULL,"
        "agi     INTEGER     NOT NULL,"
        "vit     INTEGER     NOT NULL,"
        "int     INTEGER     NOT NULL,"
        "dex     INTEGER     NOT NULL,"
        "luck    INTEGER     NOT NULL,"
        "FOREIGN KEY (user_id) REFERENCES tmw_accounts(id),"
        "FOREIGN KEY (map_id)  REFERENCES tmw_maps(id)"
#elif defined (POSTGRESQL_SUPPORT)
        "id      SERIAL      PRIMARY KEY,"
        "user_id INTEGER     NOT NULL,"
        "name    TEXT        NOT NULL UNIQUE,"
        // general information about the character
        "gender  INTEGER     NOT NULL,"
        "hair_style  INTEGER     NOT NULL,"
        "hair_color  INTEGER     NOT NULL,"
        "level   INTEGER     NOT NULL,"
        "money   INTEGER     NOT NULL,"
        // location on the map
        "x       INTEGER     NOT NULL,"
        "y       INTEGER     NOT NULL,"
        "map_id  INTEGER     NOT NULL,"
        // stats
        "str     INTEGER     NOT NULL,"
        "agi     INTEGER     NOT NULL,"
        "vit     INTEGER     NOT NULL,"
        "int     INTEGER     NOT NULL,"
        "dex     INTEGER     NOT NULL,"
        "luck    INTEGER     NOT NULL,"
        "FOREIGN KEY (user_id) REFERENCES tmw_accounts(id),"
        "FOREIGN KEY (map_id)  REFERENCES tmw_maps(id)"
#endif
    ");"
);


/**
 * TABLE: tmw_items.
 *
 * Notes:
 *     - amount: indicates how many items of the same kind can stack.
 *     - state: (optional) item state saved by script.
 */
const std::string ITEMS_TBL_NAME("tmw_items");
const std::string SQL_ITEMS_TABLE(
    "CREATE TABLE tmw_items ("
#if defined (MYSQL_SUPPORT)
        "id     SMALLINT PRIMARY KEY AUTO_INCREMENT,"
        "amount TINYINT  UNSIGNED NOT NULL,"
        "type   TINYINT  UNSIGNED NOT NULL,"
        "state  TEXT,"
        "INDEX (id)"
#elif defined (SQLITE_SUPPORT)
        "id     INTEGER  PRIMARY KEY,"
        "amount INTEGER  NOT NULL,"
        "type   INTEGER  NOT NULL,"
        "state  TEXT"
#elif defined (POSTGRESQL_SUPPORT)
        "id     SERIAL   PRIMARY KEY,"
        "amount INTEGER  NOT NULL,"
        "type   INTEGER  NOT NULL,"
        "state  TEXT"
#endif
    ");"
);


/**
 * TABLE: tmw_world_items.
 *
 * Notes:
 *     - store items on the ground in the game world.
 */
const std::string WORLD_ITEMS_TBL_NAME("tmw_world_items");
// NOTE: Problem here with primary key (only one type of item is allowed on the same map at one time).
const std::string SQL_WORLD_ITEMS_TABLE(
    "CREATE TABLE tmw_world_items ("
#if defined (MYSQL_SUPPORT)
        "id        SMALLINT UNSIGNED NOT NULL,"
        // location on the map
        "x         SMALLINT UNSIGNED NOT NULL,"
        "y         SMALLINT UNSIGNED NOT NULL,"
        "map_id    TINYINT  NOT NULL,"
        // time to die (UNIX time)
        "deathtime INTEGER  UNSIGNED NOT NULL,"
        "PRIMARY KEY (id, map_id),"
        "FOREIGN KEY (id)     REFERENCES tmw_items(id),"
        "FOREIGN KEY (map_id) REFERENCES tmw_maps(id)"
#elif defined (SQLITE_SUPPORT)
        "id        INTEGER  NOT NULL,"
        // location on the map
        "x         INTEGER  NOT NULL,"
        "y         INTEGER  NOT NULL,"
        "map_id    INTEGER  NOT NULL,"
        // time to die (UNIX time)
        "deathtime INTEGER  NOT NULL,"
        "PRIMARY KEY (id, map_id),"
        "FOREIGN KEY (id)     REFERENCES tmw_items(id),"
        "FOREIGN KEY (map_id) REFERENCES tmw_maps(id)"
#elif defined (POSTGRESQL_SUPPORT)
        "id        INTEGER  NOT NULL,"
        // location on the map
        "x         INTEGER  NOT NULL,"
        "y         INTEGER  NOT NULL,"
        "map_id    INTEGER  NOT NULL,"
        // time to die (UNIX time)
        "deathtime INTEGER  NOT NULL,"
        "PRIMARY KEY (id, map_id),"
        "FOREIGN KEY (id)     REFERENCES tmw_items(id),"
        "FOREIGN KEY (map_id) REFERENCES tmw_maps(id)"
#endif
    ");"
);


/**
 * TABLE: tmw_inventories.
 */
const std::string INVENTORIES_TBL_NAME("tmw_inventories");
const std::string SQL_INVENTORIES_TABLE(
    "CREATE TABLE tmw_inventories ("
#if defined (MYSQL_SUPPORT)
        "id       SMALLINT NOT NULL,"
        "owner_id INTEGER  NOT NULL,"
        "amount   SMALLINT NOT NULL,"
        "equipped TINYINT  NOT NULL,"
        "FOREIGN KEY (id)       REFERENCES tmw_items(id),"
        "FOREIGN KEY (owner_id) REFERENCES tmw_characters(id)"
#elif defined (SQLITE_SUPPORT)
        "id       INTEGER  NOT NULL,"
        "owner_id INTEGER  NOT NULL,"
        "amount   INTEGER  NOT NULL,"
        "equipped INTEGER  NOT NULL,"
        "FOREIGN KEY (id)     REFERENCES tmw_items(id),"
        "FOREIGN KEY (owner_id) REFERENCES tmw_characters(id)"
#elif defined (POSTGRESQL_SUPPORT)
        "id       INTEGER  NOT NULL,"
        "owner_id INTEGER  NOT NULL,"
        "amount   INTEGER  NOT NULL,"
        "equipped INTEGER  NOT NULL,"
        "FOREIGN KEY (id)       REFERENCES tmw_items(id),"
        "FOREIGN KEY (owner_id) REFERENCES tmw_characters(id)"
#endif
    ");"
);

/**
 * TABLE: tmw_channels.
 * Keeps opened public Channel list
 */
const std::string CHANNELS_TBL_NAME("tmw_channels");
const std::string SQL_CHANNELS_TABLE(
    "CREATE TABLE tmw_channels ("
#if defined (MYSQL_SUPPORT)
        "id            INTEGER     PRIMARY KEY,"
        "name          VARCHAR(32) NOT NULL UNIQUE,"
        "announcement  VARCHAR(256) NOT NULL,"
        "password      VARCHAR(32) NOT NULL,"
        "privacy       TINYINT     NOT NULL"
#elif defined (SQLITE_SUPPORT)
        "id      INTEGER     PRIMARY KEY,"
        "name    TEXT        NOT NULL UNIQUE,"
        "announcement    TEXT NOT NULL,"
        "password        TEXT NOT NULL,"
        "privacy         INTEGER NOT NULL"
#elif defined (POSTGRESQL_SUPPORT)
        "id      SERIAL      PRIMARY KEY,"
        "name    TEXT        NOT NULL UNIQUE,"
        "announcement    TEXT NOT NULL,"
        "password        TEXT NOT NULL,"
        "privacy         INTEGER NOT NULL"
#endif
    ");"
);


} // anonymous namespace


#endif // _TMWSERV_DALSTORAGE_SQL_H_
