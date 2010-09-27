include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/base \
  $(NULL)

check_PROGRAMS = \
  asyncwriter_test \
  buffer_test \
  configuration_test \
  filereaderwriter_test \
  files_test \
  hasher_test \
  report_test \
  $(NULL)

asyncwriter_test_SOURCES = asyncwriter_test.cpp
buffer_test_SOURCES = buffer_test.cpp
configuration_test_SOURCES = configuration_test.cpp
files_test_SOURCES = files_test.cpp
hasher_test_SOURCES = hasher_test.cpp
report_test_SOURCES = report_test.cpp
filereaderwriter_test_SOURCES = filereaderwriter_test.cpp

LDADD = $(top_builddir)/base/libhtools.la

check_SUCCESSES = \
  buffer.done \
  configuration.done \
  filereaderwriter.done \
  hasher.done \
  asyncwriter.done \
  files.done \
  report.done \
  $(NULL)

EXTRA_DIST = \
  asyncwriter.exp \
  buffer.exp \
  configuration.exp \
  files.exp \
  filereaderwriter.exp \
  hasher.exp \
  report.exp \
  $(NULL)
