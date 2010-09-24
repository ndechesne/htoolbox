SUBDIRS = . test

AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  $(NULL)

lib_LTLIBRARIES = libhtools.la

libhtools_la_LDFLAGS= -version-info $(LIB_VERSION)

libhtools_la_SOURCES = \
  configuration.cpp \
  files.cpp \
  filereader.cpp \
  filewriter.cpp \
  line.cpp \
  md5sumhasher.cpp \
  report.cpp \
  share.cpp \
  unzipreader.cpp \
  zipwriter.cpp \
  $(NULL)

EXTRA_DIST = \
  buffer.h \
  configuration.h \
  filereader.h \
  filewriter.h \
  files.h \
  ireader.h \
  iwriter.h \
  line.h \
  md5sumhasher.h \
  share.h \
  unzipreader.h \
  zipwriter.h \
  $(NULL)
