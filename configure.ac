AC_PREREQ(2.59)
AC_INIT([TMW Server], [0.0.1], [b_lindeijer@users.sourceforge.net])
AC_CONFIG_HEADERS([config.h:config.h.in])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([physfs], [PHYSFS_init])
AC_CHECK_LIB([SDL_net], [SDLNet_Init])

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
