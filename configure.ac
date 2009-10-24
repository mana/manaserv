AC_PREREQ(2.59)
AC_INIT([Mana Server], [1.0.0], [themanaworld-devel@lists.sourceforge.net], [manaserv])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_RANLIB
AC_PROG_INSTALL


# Checks for libraries.

AC_CHECK_LIB([physfs], [PHYSFS_init], ,
AC_MSG_ERROR([ *** Unable to find PhysFS library (icculus.org/physfs/)]))

AC_CHECK_LIB([enet], [enet_initialize], ,
AC_MSG_ERROR([ *** Unable to find enet library (enet.bespin.org)]))

PKG_CHECK_MODULES(XML2, [libxml-2.0 >= 2.4])
CXXFLAGS="$CXXFLAGS $XML2_CFLAGS"
LIBS="$LIBS $XML2_LIBS"


# Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS([stdint.h])

# Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit])


# Checks for the storage backend.
AC_ARG_WITH(
    [storage-backend],
    AS_HELP_STRING(
        [--with-storage-backend=ARG],
        [use storage backend [[ARG=mysql,postgresql,sqlite]]
        [(default=sqlite)]]
    ),
    [if test "$withval" = "yes"; then
         # default is sqlite.
         with_storage_backend="sqlite"
     elif test "$withval" = "no"; then
         AC_MSG_ERROR([$PACKAGE_NAME cannot run without a storage backend.])
     else
         with_storage_backend="$withval"
     fi],
    [with_storage_backend="sqlite"]
)

if test "$with_storage_backend" = "mysql"; then
    # use mysql_config to check libmysqlclient.
    # MYSQLCLIENT_CFLAGS and MYSQLCLIENT_LIBS are set by AC_CHECK_LIB_MANA.
    AC_CHECK_LIB_MANA([mysqlclient], [4.1.0], [mysql_config])

    # update CXXFLAGS and LIBS.
    CXXFLAGS="$CXXFLAGS -DMYSQL_SUPPORT $MYSQLCLIENT_CFLAGS"
    LIBS="$LIBS $MYSQLCLIENT_LIBS"
elif test "$with_storage_backend" = "postgresql"; then
    # use pg_config to check libpq.
    # PQ_CFLAGS and PQ_LIBS are set by AC_CHECK_LIB_MANA.
    # NOTE: PQ_LIBS not set correctly
    AC_CHECK_LIB_MANA([pq], [7.0.0], [pg_config])

    # update CXXFLAGS and LIBS.
    CXXFLAGS="$CXXFLAGS -DPOSTGRESQL_SUPPORT $PQ_CFLAGS"
    PQ_LIBS="$PQ_LIBS -lpq"
    LIBS="$LIBS $PQ_LIBS"
elif test "$with_storage_backend" = "sqlite"; then
    # use pkg-config to check libsqlite3.
    # SQLITE_CFLAGS and SQLITE_LIBS are set by PKG_CHECK_MODULES.
    PKG_CHECK_MODULES(SQLITE,[sqlite3 >= 3.0.6])
    # update CXXFLAGS and LIBS.
    CXXFLAGS="$CXXFLAGS $SQLITE_CFLAGS"
    LIBS="$LIBS $SQLITE_LIBS"
    AC_DEFINE(SQLITE_SUPPORT, [], [Define if the Sqlite back-end should be used.])
else
    # at the moment, we support only those three databases as backends.
    AC_MSG_ERROR([unknown storage backend: $with_storage_backend])
fi


# Checks for Lua support
AC_ARG_ENABLE(
    [lua-engine],
    AS_HELP_STRING([--disable-lua-engine], [do not support Lua scripts]),
    [],
    [enable_lua_engine="yes"]
)

if test "$enable_lua_engine" = "yes"; then
    PKG_CHECK_MODULES(LUA, [lua >= 5.1], ,
        [PKG_CHECK_MODULES(LUA, [lua5.1 >= 5.1], ,
            [AC_CHECK_LIB([lua-5.1], [lua_newstate], ,
                [AC_CHECK_LIB([lua], [lua_newstate], ,
                    AC_MSG_ERROR([ *** Unable to find liblua 5.1]))])])])
    CXXFLAGS="$CXXFLAGS $LUA_CFLAGS"
    LIBS="$LIBS $LUA_LIBS"
fi

AM_CONDITIONAL(BUILD_MYSQL, test "$with_storage_backend" = "mysql")
AM_CONDITIONAL(BUILD_POSTGRESQL, test "$with_storage_backend" = "postgresql")
AM_CONDITIONAL(BUILD_SQLITE, test "$with_storage_backend" = "sqlite")

AM_CONDITIONAL(BUILD_LUA, test "$enable_lua_engine" = "yes")

AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT


echo
echo "-------------------------------------------------------"
echo "  $PACKAGE_NAME will be compiled with these options:"
echo
echo "    + storage backend     : $with_storage_backend"
echo "    + Lua scripting engine: $enable_lua_engine"
echo "-------------------------------------------------------"
echo
