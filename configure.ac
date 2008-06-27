#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)

# This contains version information
AC_INIT([HBackup],[1.0a],[herve@lucidia.net])
AC_SUBST(LIB_VERSION, [1:0:1])

AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_LANG([C++])

# Compiler options
AC_ARG_ENABLE(debug,
    AC_HELP_STRING(--enable-debug, [Compile with symbols (Default = no)]),
    enable_debug=$enableval, enable_debug=no)
CXXWARN="-Wall -Wextra -Wundef -Wold-style-cast -Wconversion"
if test "$enable_debug" = "yes" ; then
    CXXFLAGS="$CXXFLAGS $CXXWARN -g -D_DEBUG"
else
    CXXFLAGS="$CXXFLAGS $CXXWARN -O3"
fi

# Checks for programs.
AC_PROG_CXX
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
AC_CHECK_LIB(pthread, deflate, [], [liberrors="yes"])
if test "x$liberrors" != "x"; then
  AC_MSG_ERROR([some libraries were missing or unusable])
fi

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h unistd.h utime.h])
AC_CHECK_HEADERS([sstream])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_MEMCMP
AC_FUNC_CHOWN
AC_FUNC_MKTIME
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_UTIME_NULL
AC_STRUCT_TM
AC_CHECK_FUNCS([gettimeofday mempcpy mkdir mkfifo regcomp select setenv])
AC_CHECK_FUNCS([strchr strdup strerror strrchr strtol strtoul tzset utime])

# Define test tools
AC_PATH_PROG(TEST_TOOL, test_tools/test_setup, test_tools/test_setup)
AC_PATH_PROG(TEST_RUN, test_tools/test_run, test_tools/test_run)

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 lib/test/Makefile
                 cli/Makefile
                 cli/test/Makefile
                 cli/man/Makefile])
AC_OUTPUT
