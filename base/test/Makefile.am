include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/base \
  $(NULL)

check_PROGRAMS = \
  buffer_test \
  configuration_test \
  filereaderwriter_test \
  files_test \
  hasher_test \
  line_test \
  report_test \
  $(NULL)

buffer_test_SOURCES = buffer_test.cpp
configuration_test_SOURCES = configuration_test.cpp
files_test_SOURCES = files_test.cpp
hasher_test_SOURCES = hasher_test.cpp
line_test_SOURCES = line_test.cpp
report_test_SOURCES = report_test.cpp
filereaderwriter_test_SOURCES = filereaderwriter_test.cpp

LDADD = $(top_builddir)/base/libhtools.la

check_SUCCESSES = \
  buffer.done \
  configuration.done \
  line.done \
  filereaderwriter.done \
  hasher.done \
  files.done \
  report.done \
  $(NULL)

EXTRA_DIST = \
  buffer.exp \
  configuration.exp \
  files.exp \
  filereaderwriter.exp \
  hasher.exp \
  line.exp \
  report.exp \
  $(NULL)
