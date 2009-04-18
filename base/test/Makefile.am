include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I$(top_srcdir)/base \
  $(NULL)

check_PROGRAMS = \
  buffer_test \
  files_test \
  line_test \
  $(NULL)

buffer_test_SOURCES = buffer_test.cpp
files_test_SOURCES = files_test.cpp
line_test_SOURCES = line_test.cpp

LDADD = $(top_builddir)/base/libhtools.la

check_SUCCESSES = \
  buffer.done \
  line.done \
  files.done \
  $(NULL)

EXTRA_DIST = \
  buffer.exp \
  files.exp \
  line.exp \
  $(NULL)
