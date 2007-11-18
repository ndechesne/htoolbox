AM_CXXFLAGS = -I@top_srcdir@/lib

check_PROGRAMS = \
	clients_test \
	conditions_test \
	cvs_parser_test \
	db_test \
	files_test \
	filters_test \
	interface_test \
	list_test \
	parsers_test \
	paths_test \
	strings_test

strings_test_SOURCES = strings_test.cpp
files_test_SOURCES = files_test.cpp
conditions_test_SOURCES = conditions_test.cpp
filters_test_SOURCES = filters_test.cpp
parsers_test_SOURCES = parsers_test.cpp
cvs_parser_test_SOURCES = cvs_parser_test.cpp
list_test_SOURCES = list_test.cpp
db_test_SOURCES = db_test.cpp
paths_test_SOURCES = paths_test.cpp
clients_test_SOURCES = clients_test.cpp
interface_test_SOURCES = interface_test.cpp

LDADD = @top_srcdir@/lib/libhbackup.la

check_SUCCESSES = \
	strings.done \
	files.done \
	conditions.done \
	filters.done \
	parsers.done \
	cvs_parser.done \
	list.done \
	db.done \
	paths.done \
	clients.done \
	interface.done

check-local: $(check_SUCCESSES)

CLEANFILES = $(check_SUCCESSES)

clean-local:
	@echo "Calling test tool to clean test environment"
	@@top_srcdir@/$(TEST_TOOL) clean

# Rules
%.done: %_test %.exp @top_srcdir@/$(TEST_RUN) @top_srcdir@/$(TEST_TOOL)
	@echo "RUN	$<"
	@@top_srcdir@/$(TEST_RUN) `basename $@ .done` && touch $@
