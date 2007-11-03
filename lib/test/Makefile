AR = ar
RANLIB = ranlib
CXXFLAGS = -Wall -g -ansi -I..
LDFLAGS = -lssl -lz

TEST_TOOL = ../../test_tools/test_setup
TEST_RUN = ../../test_tools/test_run

all:
install:

check:  strings.done \
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

clean:
	@rm -f *.[oa] *~ *.out *.err *.all *.done *_test zcop*
	@$(TEST_TOOL) clean

# Rules
%_test.o: %_test.cpp ../.libs/libhbackup.a
	@echo "CXX	$<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%_test: %_test.o ../.libs/libhbackup.a
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^

%.done: %_test %.exp $(TEST_RUN) $(TEST_TOOL)
	@echo "RUN	$<"
	@$(TEST_RUN) `basename $@ .done` && touch $@
