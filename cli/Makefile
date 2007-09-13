MAJOR = 0
MINOR = 2
BUGFIX = 0

INCLUDES = ../lib

AR ?= ar
RANLIB ?= ranlib
STRIP ?= strip
CXXFLAGS ?= -Wall -O2 -ansi -I$(INCLUDES) -DVERSION_MAJOR=${MAJOR} \
	-DVERSION_MINOR=${MINOR} -DVERSION_BUGFIX=${BUGFIX} -DBUILD=0
LDFLAGS ?= -lssl -lz
PREFIX ?= /usr/local

all: hbackup

install: hbackup
	@echo "INSTALL	$<"
	@mkdir -p ${PREFIX}/bin
	@cp $^ ${PREFIX}/bin
	@strip ${PREFIX}/bin/$^

clean:
	@rm -f *.[oa] *~ hbackup
	@${MAKE} -C test clean

check: all
	@${MAKE} -C test

# Dependencies
hbackup: ../lib/libhbackup.a

hbackup.o: ../lib/hbackup.h

# Rules
%.o: %.cpp
	@echo "CXX	$<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%: %.o
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^
	@$(STRIP) $@
