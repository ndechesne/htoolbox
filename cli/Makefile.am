MAJOR = 0
MINOR = 2
BUGFIX = 0

SUBDIRS = test

bin_PROGRAMS = hbackup
hbackup_SOURCES = hbackup.cpp
LDADD = @top_srcdir@/lib/libhbackup.la
AM_CXXFLAGS = -Wall -ansi -I@top_srcdir@/lib -DVERSION_MAJOR=${MAJOR} \
	-DVERSION_MINOR=${MINOR} -DVERSION_BUGFIX=${BUGFIX} -DBUILD=0
AM_LDFLAGS = -lssl -lz
