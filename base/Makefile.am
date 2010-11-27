SUBDIRS = . test

AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  $(NULL)

lib_LTLIBRARIES = libhtools.la

libhtools_la_LDFLAGS= -version-info $(LIB_VERSION)

libhtools_la_SOURCES = \
  asyncwriter.cpp \
  configuration.cpp \
  files.cpp \
  filereaderwriter.cpp \
  hasher.cpp \
  job_queue.cpp \
  linereaderwriter.cpp \
  multiwriter.cpp \
  report.cpp \
  share.cpp \
  unzipreader.cpp \
  work_scheduler.cpp \
  zipwriter.cpp \
  $(NULL)

EXTRA_DIST = \
  asyncwriter.h \
  buffer.h \
  configuration.h \
  filereaderwriter.h \
  files.h \
  hasher.h \
  ireaderwriter.h \
  job_queue.h \
  linereaderwriter.h \
  multiwriter.h \
  share.h \
  stackhelper.h \
  unzipreader.h \
  work_scheduler.h \
  zipwriter.h \
  $(NULL)
