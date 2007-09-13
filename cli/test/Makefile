AR := ar
RANLIB := ranlib
CXXFLAGS := -Wall -O2 -ansi -I..
LDFLAGS := -lssl -lz

all: test

test:	hbackup.done

clean:
	@rm -f *.[oa] *~ *.all *.done 
	@../../test_tools/test_setup clean

# Rules
hbackup.done: ../hbackup hbackup.exp
	@echo "RUN	$<"
	@./hbackup_test `basename $@ .done` && touch $@
