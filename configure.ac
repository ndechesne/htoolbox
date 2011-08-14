#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)

# This contains version information
AC_INIT([HToolBox],[1.3.0],[herve@lucidia.net])
AC_SUBST(LIB_VERSION, [4:0:3])

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

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
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([localtime_r memchr memset mkdir socket strchr strdup strerror strrchr])

# Define test tools
AC_PATH_PROG(TEST_TOOL, test_tools/test_setup, test_tools/test_setup)
AC_PATH_PROG(TEST_RUN, test_tools/test_run, test_tools/test_run)

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 src/Makefile
                 test/Makefile
                 test_tools/Makefile])
AC_OUTPUT
