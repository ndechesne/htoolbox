VERSION = 0.3a

SUBDIRS = test

bin_PROGRAMS = hbackup
hbackup_SOURCES = hbackup.cpp
LDADD = @top_srcdir@/lib/libhbackup.la
AM_CXXFLAGS = -Wall -ansi -I@top_srcdir@/lib -DVERSION=\"${VERSION}\"
AM_LDFLAGS = -lssl -lz
