include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = -I$(top_srcdir)/lib

check_PROGRAMS = \
	buffer_test \
	clients_test \
	conditions_test \
	configuration_test \
	cvs_parser_test \
	data_test \
	db_test \
	files_test \
	filters_test \
	interface_test \
	line_test \
	list_test \
	owner_test \
	parsers_test \
	paths_test \
	report_test \
	svn_parser_test

buffer_test_SOURCES = buffer_test.cpp
clients_test_SOURCES = clients_test.cpp
conditions_test_SOURCES = conditions_test.cpp
configuration_test_SOURCES = configuration_test.cpp
cvs_parser_test_SOURCES = cvs_parser_test.cpp
data_test_SOURCES = data_test.cpp
db_test_SOURCES = db_test.cpp
files_test_SOURCES = files_test.cpp
filters_test_SOURCES = filters_test.cpp
interface_test_SOURCES = interface_test.cpp
line_test_SOURCES = line_test.cpp
list_test_SOURCES = list_test.cpp
owner_test_SOURCES = owner_test.cpp
parsers_test_SOURCES = parsers_test.cpp
paths_test_SOURCES = paths_test.cpp
report_test_SOURCES = report_test.cpp
svn_parser_test_SOURCES = svn_parser_test.cpp

LDADD = $(top_srcdir)/lib/libhbackup.la

check_SUCCESSES = \
	buffer.done \
	line.done \
	files.done \
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
	interface.done

EXTRA_DIST = \
	buffer.exp \
	clients.exp \
	conditions.exp \
	configuration.exp \
	cvs_parser.exp \
	data.exp \
	db.exp \
	files.exp \
	filters.exp \
	interface.exp \
	line.exp \
	list.exp \
	owner.exp \
	parsers.exp \
	paths.exp \
	report.exp \
	svn_parser.exp
