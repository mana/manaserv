dnl $Id$

dnl @synopsis AC_CHECK_LIB_MANA(
dnl               LIBRARY
dnl               [, MINIMUM-VERSION
dnl               [, LIBRARY-CONFIG-EXE
dnl               [, ACTION-IF-FOUND
dnl               [, ACTION-IF-NOT-FOUND ]]]]
dnl           )
dnl
dnl This function runs a LIBRARY-config script (or LIBRARY-CONFIG-EXE if
dnl specified) and defines LIBRARY_CFLAGS and LIBRARY_LIBS.
dnl
dnl The script must support `--cflags' and `--libs' args.
dnl If MINIMUM-VERSION is specified, the script must also support the
dnl `--version' arg.
dnl If the `--with-library-[exec-]prefix' arguments to ./configure are given,
dnl it must also support `--prefix' and `--exec-prefix'.
dnl (In other words, it must be like gtk-config.)
dnl
dnl Example:
dnl
dnl    AC_CHECK_LIB_MANA(foo, 1.0.0)
dnl
dnl would run `foo-config --version' and check that it is at least 1.0.0.
dnl
dnl If so, the following would then be defined:
dnl
dnl    FOO_CFLAGS to `foo-config --cflags`
dnl    FOO_LIBS   to `foo-config --libs`
dnl
dnl This function is a hack of the original ac_path_generic.m4 written by
dnl Angus Lees <gusl@cse.unsw.edu.au>.
dnl It adds LIBRARY-CONFIG-EXE so that it is possible to define `foo_config`
dnl as the script to execute instead of the default `foo-config`.

m4_include(ax_compare_version.m4)

AC_DEFUN([AC_CHECK_LIB_MANA], [
    dnl define macros to uppercase or lowercase a string.
    pushdef([UP], translit([$1], [a-z], [A-Z]))dnl
    pushdef([DOWN], translit([$1], [A-Z], [a-z]))dnl

    dnl add two options to the configure script to set the prefix and
    dnl the exec-prefix of the library.
    AC_ARG_WITH(
        DOWN-prefix,
        AS_HELP_STRING(
            [--with-DOWN-prefix=PREFIX],
            [prefix where lib$1 is installed (optional)]
        ),
        [DOWN[]_config_prefix="$withval"],
        [DOWN[]_config_prefix=""]
    )

    AC_ARG_WITH(
        DOWN-exec-prefix,
        AS_HELP_STRING(
            [--with-DOWN-exec-prefix=EPREFIX],
            [exec prefix where lib$1 is installed (optional)]
        ),
        [DOWN[]_config_exec_prefix="$withval"],
        [DOWN[]_config_exec_prefix=""]
    )

    dnl set default shell script to execute.
    ifelse(
        [$3],
        [],
        [DOWN[]_config_script="DOWN-config"],
        [DOWN[]_config_script="$3"]
    )

    dnl print an info message if we have detected the environment
    dnl variable LIBRARY_CONFIG.
    if test -n "${UP[]_CONFIG+set}"; then
        AC_MSG_NOTICE(
            [using UP[]_CONFIG=$UP[]_CONFIG found from your environment]
        )
    fi

    if test -n "$DOWN[]_config_prefix"; then
        DOWN[]_config_args=\
            "$DOWN[]_config_args --prefix=$DOWN[]_config_prefix"

        if test -z "${UP[]_CONFIG+set}"; then
            [UP[]_CONFIG=$DOWN[]_config_prefix/bin/$DOWN[]_config_script]
        fi
    fi

    if test -n "$DOWN[]_config_exec_prefix"; then
        DOWN[]_config_args=\
            "$DOWN[]_config_args --exec-prefix=$DOWN[]_config_exec_prefix"

        if test -z "${UP[]_CONFIG+set}"; then
            [UP[]_CONFIG=$DOWN[]_config_exec_prefix/bin/$DOWN[]_config_script]
        fi
    fi

    succeeded=no

    if test -z "$UP[]_CONFIG"; then
        AC_PATH_PROG(UP[]_CONFIG, $DOWN[]_config_script, [no])
    fi

    if test "$UP[]_CONFIG" = "no"; then
        echo "*** The $DOWN[]_config_script script could not be found. Make"
        echo "*** sure it is in your path, or set the UP[]_CONFIG environment"
        echo "*** variable to the full path to $DOWN[]_config_script."
    else
        ifelse(
            [$2], [],
            AC_MSG_CHECKING([for $1]),
            AC_MSG_CHECKING([for $1 - version >= $2])
        )

        if test -x "$UP[]_CONFIG"; then
            ifelse(
                [$2], [], [],
                [DOWN[]_version=`$UP[]_CONFIG $DOWN[]_config_args --version`

                AX_COMPARE_VERSION(
                    [$DOWN[]_version], [ge], [$2],
                    [],
                    [AC_MSG_RESULT([no])

                    UP[]_CFLAGS=""
                    UP[]_LIBS=""

                    echo "***"
                    echo "*** If you have already installed a sufficiently new"
                    echo "*** version, this error probably means that the wrong"
                    echo "*** copy of the $DOWN[]_config_script shell script is"
                    echo "*** being found in your path."
                    echo "***"

                    AC_MSG_ERROR([found $DOWN[]_version])
                    ]
                )]
            )

            AC_MSG_RESULT(yes)
            succeeded="yes"

            AC_MSG_CHECKING(UP[]_CFLAGS)
            UP[]_CFLAGS=`$UP[]_CONFIG $DOWN[]_config_args --cflags`
            AC_MSG_RESULT($UP[]_CFLAGS)

            AC_MSG_CHECKING(UP[]_LIBS)
            UP[]_LIBS=`$UP[]_CONFIG $DOWN[]_config_args --libs`
            AC_MSG_RESULT($UP[]_LIBS)
        else
            AC_MSG_RESULT([could not execute $UP[]_CONFIG])

            echo "***"
            echo "*** The $UP[]_CONFIG shell script does not exist or"
            echo "*** is not executable. Please check if the file exists and"
            echo "*** is executable or update your UP[]_CONFIG environment"
            echo "*** variable so that it points to an existing DOWN-config"
            echo "*** shell script."
            echo "***"
        fi
    fi

    dnl define output variables.
    AC_SUBST(UP[]_CFLAGS)
    AC_SUBST(UP[]_LIBS)

    if test "$succeeded" = "yes"; then
        ifelse([$4], [], :, [$4])
    else
        ifelse(
            [$5],
            [],
            AC_MSG_ERROR([library requirements (>=$2) not met.]),
            [$5]
        )
    fi

    dnl undefine macros.
    popdef([UP])
    popdef([DOWN])
])
