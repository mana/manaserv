AC_PREREQ(2.59)
AC_INIT([TMW Server], [0.0.1], [b_lindeijer@users.sourceforge.net])
AC_CONFIG_HEADERS([config.h:config.h.in])
AM_INIT_AUTOMAKE

# Check if with sqlite
AC_ARG_WITH(sqlite,[  --with-sqlite           support SQLite ] )
if test "x$with_sqlite" == "xyes"; then
    with_sqlite=yes
    
    # Check with pkg-config
    PKG_CHECK_MODULES(SQLITE, [sqlite3 >= 3.0.6], 
    [],
    [AC_MSG_ERROR([Cannot find SQLite 3])]
    )
    
    # Add define
    SQLITE_CFLAGS=" -DSQLITE_SUPPORT -DUSER_SQLITE"
    
    AC_SUBST(SQLITE_CFLAGS)
    AC_SUBST(SQLITE_LIBS)
else
    with_sqlite=no
fi

AM_CONDITIONAL(BUILD_SQLITE, test x$with_sqlite = xyes)

# Check if with Postgre
AC_ARG_WITH(postgre,[  --with-postgre          support PostgreSQL ] )
if test "x$with_postgre" == "xyes"; then
    with_postgre=yes
    
    # Check with pkg-config
    AC_CHECK_LIB([pq], [PQconnectdb], ,
                 AC_MSG_ERROR([Cannot find PostgreSQL]))

    # Add define
    POSTGRE_CFLAGS=" -DPOSTGRE_SUPPORT"
    POSTGRE_LIBS=" -lpq"
    
    AC_SUBST(POSTGRE_CFLAGS)
    AC_SUBST(POSTGRE_LIBS)
else
    with_postgre=no
fi

AM_CONDITIONAL(BUILD_POSTGRE, test x$with_postgre = xyes)

# Check if with Postgre
AC_ARG_WITH(mysql,[  --with-mysql            support MySQL ] )
if test "x$with_mysql" == "xyes"; then
    with_mysql=yes
    
    # TODO: Fix this up! (the lib might not be in the LD path)
    # Check with pkg-config
    AC_CHECK_LIB([mysqlclient], [mysql_init], ,
                 AC_MSG_ERROR([Cannot find MySQL]))

    # Add define
    MYSQL_CFLAGS=" -DMYSQL_SUPPORT `mysql_config --libs`"
    MYSQL_LIBS=" `mysql_config --libs`"
    
    AC_SUBST(MYSQL_CFLAGS)
    AC_SUBST(MYSQL_LIBS)
else
    with_mysql=no
fi

AM_CONDITIONAL(BUILD_MYSQL, test x$with_mysql = xyes)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([physfs], [PHYSFS_init])
AC_CHECK_LIB([SDL_net], [SDLNet_Init])
AC_CHECK_LIB([crypto], [EVP_Digest])
AC_CHECK_LIB([crypto], [EVP_md5])

AC_ARG_WITH(scripting,[  --with-scripting        Build with scripting])
if test "x$with_scripting" == "xyes"; then
    AC_CHECK_LIB([squirrel], [sq_open], ,
      AC_MSG_ERROR([Cannot find Squirrel library (squirrel.sf.net)]))

    with_scripting=yes
    SCRIPT_CFLAGS=' -DSCRIPT_SUPPORT'
    SCRIPT_LIBS=' -lsquirrel -lsqstdlib'
    AC_SUBST(SCRIPT_CFLAGS)
    AC_SUBST(SCRIPT_LIBS)
else
    with_scripting=no
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS([atexit])

AC_CONFIG_FILES([Makefile
                 src/Makefile])

AC_OUTPUT

AMDEP=AMDEP_TRUE

echo
echo "Building with scripting support: $with_scripting"
echo "Building with SQLite support: $with_sqlite"
echo "Building with PostgreSQL support: $with_postgre"
echo "Building with MySQL support: $with_mysql"
echo
