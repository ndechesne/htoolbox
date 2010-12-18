SUBDIRS = . test

htoolsincludedir = $(includedir)/htools

htoolsinclude_HEADERS = \
  asyncwriter.h \
  configuration.h \
  filereaderwriter.h \
  files.h \
  hasher.h \
  ifilereaderwriter.h \
  ireaderwriter.h \
  linereaderwriter.h \
  multiwriter.h \
  nullwriter.h \
  queue.h \
  report.h \
  share.h \
  stackhelper.h \
  unzipreader.h \
  work_scheduler.h \
  zipwriter.h \
  $(NULL)

lib_LTLIBRARIES = libhtools.la

libhtools_la_LDFLAGS= -version-info $(LIB_VERSION)

libhtools_la_SOURCES = \
  asyncwriter.cpp \
  configuration.cpp \
  files.cpp \
  filereaderwriter.cpp \
  hasher.cpp \
  linereaderwriter.cpp \
  multiwriter.cpp \
  queue.cpp \
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
  linereaderwriter.h \
  multiwriter.h \
  queue.h \
  share.h \
  stackhelper.h \
  unzipreader.h \
  work_scheduler.h \
  zipwriter.h \
  $(NULL)
