include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/base \
  -I$(top_srcdir)/parsers \
  -I$(top_srcdir)/lib \
  $(NULL)

check_PROGRAMS = \
  clients_test \
  conditions_test \
  configuration_test \
  cvs_parser_test \
  data_test \
  db_test \
  filters_test \
  interface_test \
  list_test \
  owner_test \
  parsers_test \
  paths_test \
  report_test \
  svn_parser_test \
  $(NULL)

clients_test_SOURCES = clients_test.cpp
conditions_test_SOURCES = conditions_test.cpp
configuration_test_SOURCES = configuration_test.cpp
cvs_parser_test_SOURCES = cvs_parser_test.cpp
data_test_SOURCES = data_test.cpp
db_test_SOURCES = db_test.cpp
filters_test_SOURCES = filters_test.cpp
interface_test_SOURCES = interface_test.cpp
list_test_SOURCES = list_test.cpp
owner_test_SOURCES = owner_test.cpp
parsers_test_SOURCES = parsers_test.cpp
paths_test_SOURCES = paths_test.cpp
report_test_SOURCES = report_test.cpp
svn_parser_test_SOURCES = svn_parser_test.cpp

LDADD = \
  $(top_builddir)/parsers/libhbackup_cvs.la \
  $(top_builddir)/parsers/libhbackup_svn.la \
  $(top_builddir)/base/libhtools.la \
  $(top_builddir)/lib/libhbackup.la \
  $(NULL)

check_SUCCESSES = \
  report.done \
  configuration.done \
  conditions.done \
  filters.done \
  parsers.done \
  cvs_parser.done \
  svn_parser.done \
  list.done \
  owner.done \
  data.done \
  db.done \
  paths.done \
  clients.done \
  interface.done \
  $(NULL)

EXTRA_DIST = \
  clients.exp \
  conditions.exp \
  configuration.exp \
  cvs_parser.exp \
  data.exp \
  db.exp \
  filters.exp \
  interface.exp \
  list.exp \
  owner.exp \
  parsers.exp \
  paths.exp \
  report.exp \
  svn_parser.exp \
  $(NULL)
