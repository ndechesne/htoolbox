AR := ar
RANLIB := ranlib
CXXFLAGS := -Wall -g -ansi -I..
LDFLAGS := -lssl -lz

all:

check:  strings.done \
	files.done \
	filters.done \
	parsers.done \
	cvs_parser.done \
	list.done \
	db.done \
	paths.done \
	clients.done

clean:
	@rm -f *.[oa] *~ *.out *.err *.all *.done *_test zcop*
	@../../test_tools/test_setup clean

# Rules
%_test.o: %_test.cpp ../.libs/libhbackup.a
	@echo "CXX	$<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%_test: %_test.o ../.libs/libhbackup.a
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^

%.done: %_test %.exp
	@echo "RUN	$<"
	@../../test_tools/test_run `basename $@ .done` && touch $@
