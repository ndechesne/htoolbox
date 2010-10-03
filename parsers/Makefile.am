AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/base \
  -I$(top_srcdir)/lib \
  $(NULL)

LIBS = $(top_builddir)/base/libhtools.la

pkglib_LTLIBRARIES = cvs_parser.la svn_parser.la

cvs_parser_la_LDFLAGS = -avoid-version -module
cvs_parser_la_SOURCES = cvs_parser.cpp

svn_parser_la_LDFLAGS = -avoid-version -module
svn_parser_la_SOURCES = svn_parser.cpp
