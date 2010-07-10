include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/base \
  $(NULL)

check_PROGRAMS = \
  buffer_test \
  configuration_test \
  files_test \
  line_test \
  report_test \
  $(NULL)

buffer_test_SOURCES = buffer_test.cpp
configuration_test_SOURCES = configuration_test.cpp
files_test_SOURCES = files_test.cpp
line_test_SOURCES = line_test.cpp
report_test_SOURCES = report_test.cpp

LDADD = $(top_builddir)/base/libhtools.la

check_SUCCESSES = \
  buffer.done \
  configuration.done \
  line.done \
  files.done \
  report.done \
  $(NULL)

EXTRA_DIST = \
  buffer.exp \
  configuration.exp \
  files.exp \
  line.exp \
  report.exp \
  $(NULL)
