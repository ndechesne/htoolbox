include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/base \
  $(NULL)

check_PROGRAMS = \
  asyncwriter_test \
  configuration_test \
  filereaderwriter_test \
  files_test \
  hasher_test \
  linereaderwriter_test \
  report_test \
  zip_unzip_test \
  $(NULL)

asyncwriter_test_SOURCES = asyncwriter_test.cpp
configuration_test_SOURCES = configuration_test.cpp
filereaderwriter_test_SOURCES = filereaderwriter_test.cpp
files_test_SOURCES = files_test.cpp
hasher_test_SOURCES = hasher_test.cpp
linereaderwriter_test_SOURCES = linereaderwriter_test.cpp
report_test_SOURCES = report_test.cpp
zip_unzip_test_SOURCES = zip_unzip_test.cpp

LDADD = $(top_builddir)/base/libhtools.la

check_SUCCESSES = \
  configuration.done \
  filereaderwriter.done \
  linereaderwriter.done \
  hasher.done \
  asyncwriter.done \
  files.done \
  report.done \
  zip_unzip.done \
  $(NULL)

EXTRA_DIST = \
  asyncwriter.exp \
  configuration.exp \
  files.exp \
  filereaderwriter.exp \
  hasher.exp \
  linereaderwriter.exp \
  report.exp \
  zip_unzip.exp \
  $(NULL)
