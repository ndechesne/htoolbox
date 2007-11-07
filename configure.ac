#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([hbackup],[0.4],[herve@lucidia.net])
AM_INIT_AUTOMAKE(hbackup, 0.2)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_LANG([C++])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PROG_RANLIB

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lssl':
AC_CHECK_LIB([ssl], [main])
# FIXME: Replace `main' with a function in `-lz':
AC_CHECK_LIB([z], [main])

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

# Define test tool
AC_PATH_PROG(TEST_TOOL, test_tools/test_setup, test_tools/test_setup)
AC_PATH_PROG(TEST_RUN, test_tools/test_run, test_tools/test_run)

AC_CONFIG_FILES([Makefile
                 cli/Makefile
                 cli/test/Makefile
                 lib/Makefile
                 lib/test/Makefile
                 man/Makefile])
AC_OUTPUT
