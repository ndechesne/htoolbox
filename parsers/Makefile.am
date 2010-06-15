AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/base \
  -I$(top_srcdir)/lib \
  $(NULL)

LIBS = $(top_builddir)/base/libhtools.la

# Plugins
lib_LTLIBRARIES = libhbackup_cvs.la libhbackup_svn.la

libhbackup_cvs_la_LDFLAGS= -version-info $(LIB_VERSION)

libhbackup_cvs_la_SOURCES = \
  cvs_parser.cpp \
  $(NULL)

libhbackup_svn_la_LDFLAGS= -version-info $(LIB_VERSION)

libhbackup_svn_la_SOURCES = \
  svn_parser.cpp \
  $(NULL)

EXTRA_DIST = \
  cvs_parser.h \
  svn_parser.h \
  $(NULL)
