AM_CXXFLAGS = \
  -I../include \
  -I$(top_srcdir)/base/include \
  -I$(top_srcdir)/lib/include \
  -I$(top_srcdir)/lib/src \
  $(NULL)

lib_LTLIBRARIES = libhbackend_client.la libhbackend_server.la

libhbackend_client_la_SOURCES = \
  data_client.cpp \
  $(NULL)

libhbackend_client_la_LDFLAGS= -version-info $(LIB_VERSION)

libhbackend_client_la_LIBADD = \
  $(top_builddir)/base/src/libhtools.la \
  $(NULL)

libhbackend_server_la_SOURCES = \
  data_server.cpp \
  $(NULL)

libhbackend_server_la_LDFLAGS= -version-info $(LIB_VERSION)

libhbackend_server_la_LIBADD = \
  $(top_builddir)/base/src/libhtools.la \
  $(top_builddir)/lib/src/libhbackup.la \
  $(NULL)

bin_PROGRAMS = hbackend

hbackend_LDADD = \
  $(top_builddir)/base/src/libhtools.la \
  $(top_builddir)/lib/src/libhbackup.la \
  ./libhbackend_server.la \
  $(NULL)

hbackend_SOURCES = \
  main.cpp \
  $(NULL)
