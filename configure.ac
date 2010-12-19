#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)

# This contains version information
AC_INIT([HBackup],[1.2a],[herve@lucidia.net])
AC_DEFINE([HBACKUP_CONFIG], ["/etc/hbackup/config"],
  [Default configuration file path])
AC_SUBST(LIB_VERSION, [3:0:2])

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AC_DISABLE_STATIC
AC_LANG([C++])

# Compiler options
CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wundef -Wold-style-cast -Wconversion"
AC_ARG_ENABLE(debug,
    AC_HELP_STRING(--enable-debug, [Compile with symbols (Default = yes)]),
    enable_debug=$enableval, enable_debug=yes)
if test "$enable_debug" = "yes" ; then
    CXXFLAGS="$CXXFLAGS -g -D_DEBUG"
else
    CXXFLAGS="$CXXFLAGS -O3"
fi

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for libraries.
liberrors=""
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB(ssl, SSL_CTX_new, [], [liberrors="yes"])
AC_CHECK_LIB(z, deflate, [], [liberrors="yes"])
AC_CHECK_LIB(pthread, deflate, [], [liberrors="yes"])
if test "x$liberrors" != "x"; then
  AC_MSG_ERROR([some libraries were missing or unusable])
fi

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/time.h unistd.h utime.h])
AC_CHECK_HEADERS([sstream])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getdelim localtime_r memchr memset mkdir mkfifo regcomp setenv strcasecmp strchr strdup strerror strncasecmp strrchr tzset utime])

# Define test tools
AC_PATH_PROG(TEST_TOOL, test_tools/test_setup, test_tools/test_setup)
AC_PATH_PROG(TEST_RUN, test_tools/test_run, test_tools/test_run)

AC_CONFIG_FILES([Makefile
                 backend/Makefile
                 backend/src/Makefile
                 backend/test/Makefile
                 base/Makefile
                 base/include/Makefile
                 base/src/Makefile
                 base/test/Makefile
                 cli/Makefile
                 cli/man/Makefile
                 cli/src/Makefile
                 cli/test/Makefile
                 doc/Makefile
                 lib/Makefile
                 lib/include/Makefile
                 lib/src/Makefile
                 lib/test/Makefile
                 parsers/Makefile
                 test_tools/Makefile])
AC_OUTPUT
