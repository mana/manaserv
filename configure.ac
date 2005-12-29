dnl $Id$


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

AC_CHECK_LIB([crypto], [EVP_md5], ,
AC_MSG_ERROR([ *** Unable to find libcrypto library]))

AC_CHECK_LIB([physfs], [PHYSFS_init], ,
AC_MSG_ERROR([ *** Unable to find PhysFS library (icculus.org/physfs/)]))

AC_CHECK_LIB([SDL_net], [SDLNet_Init], ,
AC_MSG_ERROR([ *** Unable to find SDL_net library]))

PKG_CHECK_MODULES(XML2, [libxml-2.0 >= 2.4])
CXXFLAGS="$CXXFLAGS $XML2_CFLAGS"
LIBS="$LIBS $XML2_LIBS"

AC_CHECK_LIB_TMW([SDL], [1.2.0])
# update CXXFLAGS and LIBS.
# SDL_CFLAGS and SDL_LIBS are set by AC_CHECK_LIB_TMW.
CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"


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
    # MYSQLCLIENT_CFLAGS and MYSQLCLIENT_LIBS are set by AC_CHECK_LIB_TMW.
    AC_CHECK_LIB_TMW([mysqlclient], [4.1.0], [mysql_config])

    # update CXXFLAGS and LIBS.
    CXXFLAGS="$CXXFLAGS -DMYSQL_SUPPORT $MYSQLCLIENT_CFLAGS"
    LIBS="$LIBS $MYSQLCLIENT_LIBS"
elif test "$with_storage_backend" = "postgresql"; then
    # use pg_config to check libpq.
    # PQ_CFLAGS and PQ_LIBS are set by AC_CHECK_LIB_TMW.
    # NOTE: PQ_LIBS not set correctly
    AC_CHECK_LIB_TMW([pq], [7.0.0], [pg_config])

    # update CXXFLAGS and LIBS.
    CXXFLAGS="$CXXFLAGS -DPOSTGRESQL_SUPPORT $PQ_CFLAGS"
    PQ_LIBS="$PQ_LIBS -lpq"
    LIBS="$LIBS $PQ_LIBS"
elif test "$with_storage_backend" = "sqlite"; then
    # use pkg-config to check libsqlite3.
    # SQLITE_CFLAGS and SQLITE_LIBS are set by PKG_CHECK_MODULES.
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
    # AC_MSG_ERROR([sorry, $with_scripting_engine is not supported yet])
    AC_CHECK_LIB([ruby], [ruby_init])
    AC_PATH_PROG(swig_path, swig, no)
    if test "$swig_path" = "no"; then
       AC_MSG_ERROR(Could not find swig in PATH)
    fi
    SWIG="$swig_path"
    SWIGFLAGS="-c++ -ruby -minherit"
    AC_SUBST(SWIG)
    AC_SUBST(SWIGFLAGS)
    CXXFLAGS="$CXXFLAGS -DSCRIPT_SUPPORT -DRUBY_SUPPORT -I/usr/lib/ruby/1.8/linux/"
    LIBS="$LIBS -lruby"
elif test "$with_scripting_engine" = "squirrel"; then
    AC_CHECK_LIB([squirrel], [sq_open])

    # update CXXFLAGS and LIBS
    CXXFLAGS="$CXXFLAGS -DSCRIPT_SUPPORT -DSQUIRREL_SUPPORT"
    # there is no need to append -lsquirrel as it is already done by
    # AC_CHECK_LIB
    LIBS="$LIBS -lsqstdlib"
elif test "$with_scripting_engine" != "no"; then
    AC_MSG_ERROR([unknown scripting engine: $with_scripting_engine])
fi


# Checks for the unit tests.
AC_ARG_ENABLE(
    [unit-tests],
    AS_HELP_STRING([--enable-unit-tests], [compile the unit tests]),
    [if test "$enableval" = "yes"; then
         enable_unit_tests="$enableval"

         # use cppunit-config to check the libcppunit.
         # CPPUNIT_CFLAGS and CPPUNIT_LIBS are set by AC_CHECK_LIB_TMW.
         AC_CHECK_LIB_TMW([cppunit], [1.10.2])
     else
         enable_unit_tests="no"
     fi],
    [enable_unit_tests="no"]
)


AM_CONDITIONAL(BUILD_MYSQL, test "$with_storage_backend" = "mysql")
AM_CONDITIONAL(BUILD_POSTGRESQL, test "$with_storage_backend" = "postgresql")
AM_CONDITIONAL(BUILD_SQLITE, test "$with_storage_backend" = "sqlite")

AM_CONDITIONAL(BUILD_RUBY, test "$with_scripting_engine" = "ruby")
AM_CONDITIONAL(BUILD_SQUIRREL, test "$with_scripting_engine" = "squirrel")

AM_CONDITIONAL(UNIT_TESTS_YES, test "$enable_unit_tests" = "yes")


AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/tests/Makefile])

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
