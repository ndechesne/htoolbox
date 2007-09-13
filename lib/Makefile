AR := ar
RANLIB := ranlib
STRIP := strip
CXXFLAGS := -Wall -O2 -ansi
LDFLAGS := -lssl -lz
PREFIX := /usr/local

all: libhbackup.a

install: all

clean:
	@rm -f *.[oa] *~
	@${MAKE} -C test clean

check: all
	@${MAKE} -C test

# Dependencies
libhbackup.a: interface.o clients.o db.o list.o filters.o paths.o files.o \
	cvs_parser.o strings.o

clients.o: strings.h files.h filters.h parsers.h cvs_parser.h list.h db.h \
	paths.h clients.h hbackup.h
cvs_parser.o: strings.h files.h parsers.h cvs_parser.h
db.o: strings.h files.h list.h db.h hbackup.h
files.o: strings.h files.h hbackup.h
filters.o: strings.h files.h filters.h
interface.o: strings.h files.h db.h clients.h hbackup.h
list.o: strings.h files.h list.h hbackup.h
paths.o: strings.h files.h filters.h parsers.h cvs_parser.h list.h db.h \
	paths.h hbackup.h
strings.o: strings.h

# Rules
%.o: %.cpp
	@echo "CXX	$<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%.a:
	@echo "AR	$@"
	@$(AR) cru $@ $^
	@echo "RANLIB	$@"
	@$(RANLIB) $@

%: %.o
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^
	@$(STRIP) $@
