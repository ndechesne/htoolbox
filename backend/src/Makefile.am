AM_CXXFLAGS = \
  -I../include \
  -I$(top_srcdir)/base/include \
  -I$(top_srcdir)/lib/include \
  -I$(top_srcdir)/lib/src \
  $(NULL)

LDADD = \
  @top_srcdir@/base/src/libhtools.la \
  @top_srcdir@/lib/src/libhbackup.la \
  $(NULL)

bin_PROGRAMS = hbackend

hbackend_SOURCES = \
  main.cpp \
  protocol.cpp \
  $(NULL)

EXTRA_DIST = \
  protocol.h \
  $(NULL)
