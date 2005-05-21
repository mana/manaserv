AC_PREREQ(2.59)
AC_INIT([TMW Server], [0.0.1], [b_lindeijer@users.sourceforge.net])
AC_CONFIG_HEADERS([config.h:config.h.in])
AM_INIT_AUTOMAKE

# Check with pkg-config
PKG_CHECK_MODULES(SQLITE, [sqlite3 >= 3.0.6], 
    [],
    [AC_MSG_ERROR([Cannot find SQLite 3])]
)

AC_SUBST(SQLITE_CFLAGS)
AC_SUBST(SQLITE_LIBS)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([physfs], [PHYSFS_init])
AC_CHECK_LIB([SDL_net], [SDLNet_Init])

AC_ARG_WITH(scripting,[  --with-scripting       Build with scripting])
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

echo
echo "Building with scripting support: $with_scripting"
