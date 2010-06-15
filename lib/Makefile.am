SUBDIRS = . test

AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/base \
  -I$(top_srcdir)/parsers \
  $(NULL)

LIBS = \
  $(top_builddir)/base/libhtools.la \
  $(top_builddir)/parsers/libhbackup_cvs.la \
  $(top_builddir)/parsers/libhbackup_svn.la \
  $(NULL)

# Library
lib_LTLIBRARIES = libhbackup.la
libhbackup_la_LDFLAGS= -version-info $(LIB_VERSION)

libhbackup_la_SOURCES = \
  clients.cpp \
  conditions.cpp \
  configuration.cpp \
  data.cpp \
  db.cpp \
  filters.cpp \
  interface.cpp \
  list.cpp \
  missing.cpp \
  owner.cpp \
  parsers.cpp \
  paths.cpp \
  report.cpp \
  $(NULL)

EXTRA_DIST = \
  clients.h \
  compdata.h \
  conditions.h \
  configuration.h \
  data.h \
  db.h \
  filters.h \
  list.h \
  missing.h \
  opdata.h \
  owner.h \
  parsers.h \
  paths.h \
  report.h \
  $(NULL)
