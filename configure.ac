#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)

# This contains version information
AC_INIT([HBackup],[0.4b],[herve@lucidia.net])
AC_SUBST(LIB_VERSION, [0:2:0])

AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_LANG([C++])

# Compiler options
AC_ARG_ENABLE(debug,
    AC_HELP_STRING(--enable-debug, [Compile with symbols (Default = no)]),
    enable_debug=$enableval, enable_debug=no)
CXXWARN="-Wall -Wundef -Wno-trigraphs -fno-strict-aliasing -fno-common"
CWARN="$CXXWARN -Wstrict-prototypes"
if test "$enable_debug" = "yes" ; then
    CFLAGS="$CFLAGS $CWARN -g -D_DEBUG"
    CXXFLAGS="$CXXFLAGS $CXXWARN -g -D_DEBUG"
else
    CFLAGS="$CFLAGS $CWARN -O2"
    CXXFLAGS="$CXXFLAGS $CXXWARN -O2"
fi

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
#AC_PROG_RANLIB

# Checks for libraries.
liberrors=""
AC_CHECK_LIB(ssl, SSL_CTX_new, [], [liberrors="yes"])
AC_CHECK_LIB(z, deflate, [], [liberrors="yes"])
if test "x$liberrors" != "x"; then
  AC_MSG_ERROR([some libraries were missing or unusable])
fi

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS(sstream)

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([mkdir mkfifo regcomp strchr strdup strerror strrchr strtoul])

# Define test tools
AC_PATH_PROG(TEST_TOOL, test_tools/test_setup, test_tools/test_setup)
AC_PATH_PROG(TEST_RUN, test_tools/test_run, test_tools/test_run)

# Use colormake if available
AC_PATH_PROG(MAKE, hmake, make)

AC_CONFIG_FILES([Makefile
                 cli/Makefile
                 cli/test/Makefile
                 lib/Makefile
                 lib/test/Makefile
                 man/Makefile])
AC_OUTPUT
