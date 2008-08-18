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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !defined (MYSQL_SUPPORT) && !defined (SQLITE_SUPPORT) && \
    !defined (POSTGRESQL_SUPPORT)
#error "(dalstorage.hpp) no database backend defined"
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


/**
 * TABLE: tmw_accounts.
 */
static char const *ACCOUNTS_TBL_NAME = "tmw_accounts";
static char const *SQL_ACCOUNTS_TABLE =
    "CREATE TABLE tmw_accounts ("
#if defined (MYSQL_SUPPORT)
        "id       INTEGER     PRIMARY KEY AUTO_INCREMENT,"
        "username VARCHAR(32) NOT NULL UNIQUE,"
        "password VARCHAR(32) NOT NULL,"
        "email    VARCHAR(64) NOT NULL,"
        "level    TINYINT     UNSIGNED NOT NULL,"
        "banned   TINYINT     UNSIGNED NOT NULL,"
//        "activation VARCHAR(32),"
        "INDEX (id)"
#error "Incorrect definition. Please fix the types."
#elif defined (SQLITE_SUPPORT)
        "id       INTEGER     PRIMARY KEY,"
        "username TEXT        NOT NULL UNIQUE,"
        "password TEXT        NOT NULL,"
        "email    TEXT        NOT NULL,"
        "level    INTEGER     NOT NULL,"
        "banned   INTEGER     NOT NULL"
//        "activation TEXT"
#elif defined (POSTGRESQL_SUPPORT)
        "id       SERIAL      PRIMARY KEY,"
        "username TEXT        NOT NULL UNIQUE,"
        "password TEXT        NOT NULL,"
        "email    TEXT        NOT NULL,"
        "level    INTEGER     NOT NULL,"
        "banned   INTEGER     NOT NULL,"
//        "activation TEXT"
#endif
    ");";


/**
 * TABLE: tmw_characters.
 *     - gender is 0 for male, 1 for female.
 */
static char const *CHARACTERS_TBL_NAME = "tmw_characters";
static char const *SQL_CHARACTERS_TABLE =
    "CREATE TABLE tmw_characters ("
#if defined (MYSQL_SUPPORT)
        "id      INTEGER     PRIMARY KEY AUTO_INCREMENT,"
        "user_id INTEGER     UNSIGNED NOT NULL,"
        "name    VARCHAR(32) NOT NULL UNIQUE,"
        // general information about the character
        "gender      TINYINT     UNSIGNED NOT NULL,"
        "hair_style  TINYINT     UNSIGNED NOT NULL,"
        "hair_color  TINYINT     UNSIGNED NOT NULL,"
        "level       INTEGER     UNSIGNED NOT NULL,"
        "char_pts    INTEGER     UNSIGNED NOT NULL,"
        "correct_pts INTEGER     UNSIGNED NOT NULL,"
        "money   INTEGER     UNSIGNED NOT NULL,"
        // location on the map
        "x       SMALLINT    UNSIGNED NOT NULL,"
        "y       SMALLINT    UNSIGNED NOT NULL,"
        "map_id  TINYINT     NOT NULL,"
        // attributes
        "str     SMALLINT    UNSIGNED NOT NULL,"
        "agi     SMALLINT    UNSIGNED NOT NULL,"
        "dex     SMALLINT    UNSIGNED NOT NULL,"
        "vit     SMALLINT    UNSIGNED NOT NULL,"
        // note: int must be backquoted as it's a MySQL keyword
        "`int`   SMALLINT    UNSIGNED NOT NULL,"
        "will    SMALLINT    UNSIGNED NOT NULL,"
        //skill experience
        "unarmedExp     INTEGER  UNSIGNED NOT NULL,"
        "knife_exp      INTEGER  UNSIGNED NOT NULL,"
        "sword_exp      INTEGER  UNSIGNED NOT NULL,"
        "polearm_exp    INTEGER  UNSIGNED NOT NULL,"
        "staff_exp      INTEGER  UNSIGNED NOT NULL,"
        "whip_exp       INTEGER  UNSIGNED NOT NULL,"
        "bow_exp        INTEGER  UNSIGNED NOT NULL,"
        "shoot_exp      INTEGER  UNSIGNED NOT NULL,"
        "mace_exp       INTEGER  UNSIGNED NOT NULL,"
        "axe_exp        INTEGER  UNSIGNED NOT NULL,"
        "thrown_exp     INTEGER  UNSIGNED NOT NULL,"

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
        "level       INTEGER     NOT NULL,"
        "char_pts    INTEGER     NOT NULL,"
        "correct_pts INTEGER     NOT NULL,"
        "money   INTEGER     NOT NULL,"
        // location on the map
        "x       INTEGER     NOT NULL,"
        "y       INTEGER     NOT NULL,"
        "map_id  INTEGER     NOT NULL,"
        // attributes
        "str     INTEGER     NOT NULL,"
        "agi     INTEGER     NOT NULL,"
        "dex     INTEGER     NOT NULL,"
        "vit     INTEGER     NOT NULL,"
        "int     INTEGER     NOT NULL,"
        "will    INTEGER     NOT NULL,"
        //skill experience
        "unarmed_exp    INTEGER  NOT NULL,"
        "knife_exp      INTEGER  NOT NULL,"
        "sword_exp      INTEGER  NOT NULL,"
        "polearm_exp    INTEGER  NOT NULL,"
        "staff_exp      INTEGER  NOT NULL,"
        "whip_exp       INTEGER  NOT NULL,"
        "bow_exp        INTEGER  NOT NULL,"
        "shoot_exp      INTEGER  NOT NULL,"
        "mace_exp       INTEGER  NOT NULL,"
        "axe_exp        INTEGER  NOT NULL,"
        "thrown_exp     INTEGER  NOT NULL,"
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

        "char_pts    INTEGER NOT NULL,"
        "correct_pts INTEGER NOT NULL,"
        "money   INTEGER     NOT NULL,"
        // location on the map
        "x       INTEGER     NOT NULL,"
        "y       INTEGER     NOT NULL,"
        "map_id  INTEGER     NOT NULL,"
        // attributes
        "str     INTEGER     NOT NULL,"
        "agi     INTEGER     NOT NULL,"
        "dex     INTEGER     NOT NULL,"
        "vit     INTEGER     NOT NULL,"
        "int     INTEGER     NOT NULL,"
        "will    INTEGER     NOT NULL,"
        //skill experience
        "unarmed_exp    INTEGER  NOT NULL,"
        "knife_exp      INTEGER  NOT NULL,"
        "sword_exp      INTEGER  NOT NULL,"
        "polearm_exp    INTEGER  NOT NULL,"
        "staff_exp      INTEGER  NOT NULL,"
        "whip_exp       INTEGER  NOT NULL,"
        "bow_exp        INTEGER  NOT NULL,"
        "shoot_exp      INTEGER  NOT NULL,"
        "mace_exp       INTEGER  NOT NULL,"
        "axe_exp        INTEGER  NOT NULL,"
        "thrown_exp     INTEGER  NOT NULL,"
        "FOREIGN KEY (user_id) REFERENCES tmw_accounts(id),"
        "FOREIGN KEY (map_id)  REFERENCES tmw_maps(id)"
#endif
    ");";


/**
 * TABLE: tmw_inventories.
 */
static char const *INVENTORIES_TBL_NAME("tmw_inventories");
static char const *SQL_INVENTORIES_TABLE =
    "CREATE TABLE tmw_inventories ("
#if defined (MYSQL_SUPPORT)
        "id       INTEGER  PRIMARY KEY AUTO_INCREMENT,"
        "owner_id INTEGER  NOT NULL,"
        "slot     SMALLINT NOT NULL,"
        "class_id INTEGER  NOT NULL,"
        "amount   SMALLINT NOT NULL,"
        "FOREIGN KEY (owner_id) REFERENCES tmw_characters(id)"
        "INDEX (id)"
#elif defined (SQLITE_SUPPORT)
        "id       INTEGER  PRIMARY KEY,"
        "owner_id INTEGER  NOT NULL,"
        "slot     INTEGER  NOT NULL,"
        "class_id INTEGER  NOT NULL,"
        "amount   INTEGER  NOT NULL,"
        "FOREIGN KEY (owner_id) REFERENCES tmw_characters(id)"
#elif defined (POSTGRESQL_SUPPORT)
        "id      SERIAL    PRIMARY KEY,"
        "owner_id INTEGER  NOT NULL,"
        "slot     INTEGER  NOT NULL,"
        "class_id INTEGER  NOT NULL,"
        "amount   INTEGER  NOT NULL,"
        "FOREIGN KEY (owner_id) REFERENCES tmw_characters(id)"
#endif
    ");";

/**
 * TABLE: tmw_guilds.
 * Store player guilds
 */
static char const *GUILDS_TBL_NAME = "tmw_guilds";
static char const *SQL_GUILDS_TABLE =
    "CREATE TABLE tmw_guilds ("
#if defined (MYSQL_SUPPORT)
        "id            INTEGER     PRIMARY KEY AUTO_INCREMENT,"
        "name          VARCHAR(32) NOT NULL UNIQUE"
#elif defined (SQLITE_SUPPORT)
        "id      INTEGER     PRIMARY KEY,"
        "name    TEXT        NOT NULL UNIQUE"
#elif defined (POSTGRESQL_SUPPORT)
        "id      SERIAL      PRIMARY KEY,"
        "name    TEXT        NOT NULL UNIQUE"
#endif
    ");";

/**
 * TABLE: tmw_guild_members.
 * Store guild members
 */
static char const *GUILD_MEMBERS_TBL_NAME = "tmw_guild_members";
static char const *SQL_GUILD_MEMBERS_TABLE =
    "CREATE TABLE tmw_guild_members ("
#if defined (MYSQL_SUPPORT)
        "guild_id       INTEGER     NOT NULL,"
        "member_id      INTEGER     NOT NULL,"
        "rights         INTEGER     NOT NULL,"
        "FOREIGN KEY (guild_id)    REFERENCES tmw_guilds(id),"
        "FOREIGN KEY (member_id)   REFERENCES tmw_characters(id)"
#elif defined (SQLITE_SUPPORT)
        "guild_id       INTEGER     NOT NULL,"
        "member_id      INTEGER     NOT NULL,"
        "rights         INTEGER     NOT NULL,"
        "FOREIGN KEY (guild_id)    REFERENCES tmw_guilds(id),"
        "FOREIGN KEY (member_id)   REFERENCES tmw_characters(id)"
#elif defined (POSTGRESQL_SUPPORT)
        "guild_id       INTEGER     NOT NULL,"
        "member_id      INTEGER     NOT NULL,"
        "rights         INTEGER     NOT NULL,"
        "FOREIGN KEY (guild_id)    REFERENCES tmw_guilds(id),"
        "FOREIGN KEY (member_id)   REFERENCES tmw_characters(id)"
#endif
    ");";

/**
 * TABLE: tmw_quests.
 */
static char const *QUESTS_TBL_NAME = "tmw_quests";
static char const *SQL_QUESTS_TABLE =
    "CREATE TABLE tmw_quests ("
#if defined (MYSQL_SUPPORT)
#error "Missing definition. Please fill the blanks."
#elif defined (SQLITE_SUPPORT)
        "owner_id INTEGER NOT NULL,"
        "name     TEXT    NOT NULL,"
        "value    TEXT    NOT NULL,"
        "FOREIGN KEY (owner_id) REFERENCES tmw_characters(id)"
#elif defined (POSTGRESQL_SUPPORT)
#error "Missing definition. Please fill the blanks."
#endif
    ");";

#endif // _TMWSERV_DALSTORAGE_SQL_H_
