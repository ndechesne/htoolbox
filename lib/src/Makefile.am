AM_CXXFLAGS = \
  -I$(top_srcdir)/base/include \
  -I../include \
  $(NULL)

# Library
lib_LTLIBRARIES = libhbackup.la

libhbackup_la_LDFLAGS= -version-info $(LIB_VERSION)

libhbackup_la_LIBADD = \
  $(top_builddir)/base/src/libhtools.la \
  -ldl \
  $(NULL)

libhbackup_la_SOURCES = \
  attributes.cpp \
  backup.cpp \
  clients.cpp \
  conditions.cpp \
  data.cpp \
  db.cpp \
  filters.cpp \
  interface.cpp \
  list.cpp \
  missing.cpp \
  owner.cpp \
  parsers.cpp \
  paths.cpp \
  $(NULL)

EXTRA_DIST = \
  attributes.h \
  backup.h \
  clients.h \
  compdata.h \
  conditions.h \
  data.h \
  db.h \
  filters.h \
  list.h \
  missing.h \
  opdata.h \
  owner.h \
  parsers.h \
  paths.h \
  $(NULL)
