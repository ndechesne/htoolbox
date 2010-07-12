SUBDIRS = . test

AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  $(NULL)

lib_LTLIBRARIES = libhtools.la

libhtools_la_SOURCES = \
  configuration.cpp \
  files.cpp \
  line.cpp \
  report.cpp \
  $(NULL)

EXTRA_DIST = \
  buffer.h \
  configuration.h \
  files.h \
  line.h \
  $(NULL)
