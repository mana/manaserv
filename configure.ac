dnl $Id$


# TODO: write an M4 PKG_CHECK_MODULES-like for the external libraries that
#       provide an executable <lib>-config (e.g. SDL, MySQL, PostgreSQL) to
#       make this file shorter, more readable and easier to maintain.


AC_PREREQ(2.59)
AC_INIT([TMW Server], [0.0.1], [b_lindeijer@users.sourceforge.net], [tmwserv])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_RANLIB
AC_PROG_INSTALL


# Checks for libraries.
AC_CHECK_LIB([crypto], [EVP_md5])
AC_CHECK_LIB([physfs], [PHYSFS_init])

# libSDL
AC_PATH_PROG(SDL_CONFIG, [sdl-config], [no])
if test "$SDL_CONFIG" = "no"; then
    AC_MSG_ERROR([The sdl-config could not be found. Please check your path.])
else
    AC_MSG_CHECKING(SDL_CFLAGS)
    SDL_CFLAGS=`$SDL_CONFIG --cflags`
    AC_MSG_RESULT($SDL_CFLAGS)

    AC_MSG_CHECKING(SDL_LIBS)
    SDL_LIBS=`$SDL_CONFIG --libs`
    AC_MSG_RESULT($SDL_LIBS)

    # update CXXFLAGS and LIBS.
    CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
    LIBS="$LIBS $SDL_LIBS"
fi

AC_CHECK_LIB([SDL_net], [SDLNet_Init])


# Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIME


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM


# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit strrchr])


# Checks for the storage backend.
AC_ARG_WITH(
    [storage-backend],
    AS_HELP_STRING(
        [--with-storage-backend=ARG],
        [use storage backend [[ARG=mysql,postgresql,sqlite]]
        [(default=sqlite)]]
    ),
    [],
    [with_storage_backend="sqlite"]
)

if test "$with_storage_backend" = "mysql"; then
    # use mysql_config to get the CFLAGS and LIBS values.
    AC_PATH_PROG(MYSQL_CONFIG, [mysql_config], [no])

    if test "$MYSQL_CONFIG" = "no"; then
        AC_MSG_ERROR(
            [The mysql_config could not be found. Please check your path.]
        )
    else
        AC_MSG_CHECKING(MYSQL_CFLAGS)
        MYSQL_CFLAGS=`$MYSQL_CONFIG --cflags`
        AC_MSG_RESULT($MYSQL_CFLAGS)

        AC_MSG_CHECKING(MYSQL_LIBS)
        MYSQL_LIBS=`$MYSQL_CONFIG --libs`
        AC_MSG_RESULT($MYSQL_LIBS)

        # update CXXFLAGS and LIBS.
        CXXFLAGS="$CXXFLAGS -DMYSQL_SUPPORT $MYSQL_CFLAGS"
        LIBS="$LIBS $MYSQL_LIBS"
    fi
elif test "$with_storage_backend" = "postgresql"; then
    # use mysql_config to get the CFLAGS and LIBS values.
    AC_PATH_PROG(POSTGRESQL_CONFIG, [pg_config], [no])

    if test "$POSTGRESQL_CONFIG" = "no"; then
        AC_MSG_ERROR(
            [The pg_config could not be found. Please check your path.]
        )
    else
        AC_MSG_CHECKING(POSTGRESQL_CFLAGS)
        POSTGRESQL_CFLAGS=`$POSTGRESQL_CONFIG --cflags`
        AC_MSG_RESULT($POSTGRESQL_CFLAGS)

        AC_MSG_CHECKING(POSTGRESQL_LIBS)
        POSTGRESQL_LIBS=`$POSTGRESQL_CONFIG --libs`
        AC_MSG_RESULT($POSTGRESQL_LIBS)

        # update CXXFLAGS and LIBS.
        CXXFLAGS="$CXXFLAGS -DPOSTGRESQL_SUPPORT $POSTGRESQL_CFLAGS"
        LIBS="$LIBS $POSTGRESQL_LIBS"
    fi
elif test "$with_storage_backend" = "sqlite"; then
    # use pkg-config to check libsqlite3.
    # SQLITE_CFLAGS and SQLITE_LIBS are set by PKG_CHECK_MODULES
    # so nothing much to do here :)
    PKG_CHECK_MODULES(SQLITE,[sqlite3 >= 3.0.6])

    # update CXXFLAGS and LIBS.
    CXXFLAGS="$CXXFLAGS -DSQLITE_SUPPORT $SQLITE_CFLAGS"
    LIBS="$LIBS $SQLITE_LIBS"
else
    # at the moment, we support only those three databases as backends.
    AC_MSG_ERROR([unknown storage backend: $with_storage_backend])
fi


# Checks for the scripting engine.
AC_ARG_WITH(
    [scripting-engine],
    AS_HELP_STRING(
        [--with-scripting-engine=ARG],
        [use scripting engine [[ARG=ruby,squirrel]] [(default=squirrel)]]
    ),
    [],
    [with_scripting_engine="no"]
)

if test "$with_scripting_engine" = "ruby"; then
    AC_MSG_ERROR([sorry, $with_scripting_engine is not supported yet])
elif test "$with_scripting_engine" = "squirrel"; then
    AC_CHECK_LIB([squirrel], [sq_open])

    # update CXXFLAGS and LIBS
    CXXFLAGS="$CXXFLAGS -DSCRIPT_SUPPORT -DSQUIRREL_SUPPORT"
    LIBS="$LIBS -lsqstdlib"
elif test ! "$with_scripting_engine" = "no"; then
    AC_MSG_ERROR([unknown scripting engine: $with_scripting_engine])
fi


# Checks for the unit tests.
AC_ARG_ENABLE(
    [unit-tests],
    AS_HELP_STRING([--enable-unit-tests], [compile the unit tests]),
    [],
    [enable_unit_tests="no"]
)


# TODO
# Configure the unit tests.
#if test "$enable_unit_tests" = "yes" -a -d $srcdir/src/tests; then
#    AC_CONFIG_SUBDIRS([src/tests])
#fi


AM_CONDITIONAL(BUILD_MYSQL, test "$with_storage_backend" = "mysql")
AM_CONDITIONAL(BUILD_POSTGRESQL, test "$with_storage_backend" = "postgresql")
AM_CONDITIONAL(BUILD_SQLITE, test "$with_storage_backend" = "sqlite")

AM_CONDITIONAL(BUILD_RUBY, test "$with_scripting_engine" = "ruby")
AM_CONDITIONAL(BUILD_SQUIRREL, test "$with_scripting_engine" = "squirrel")


AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT


echo
echo "-------------------------------------------------------"
echo "  $PACKAGE_NAME will be compiled with these options:   "
echo
echo "    + storage backend : $with_storage_backend          "
echo "    + scripting engine: $with_scripting_engine         "
echo "    + with unit tests : $enable_unit_tests             "
echo "-------------------------------------------------------"
echo
