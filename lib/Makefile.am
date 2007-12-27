SUBDIRS = . test

# Library
lib_LTLIBRARIES = libhbackup.la
libhbackup_la_LDFLAGS= -version-info $(LIB_VERSION)

libhbackup_la_SOURCES = \
	bzr_parser.h \
	clients.h \
	cvs_parser.h \
	conditions.h \
	db.h \
	files.h \
	filters.h \
	hbackup.h \
	list.h \
	parsers.h \
	paths.h \
	strings.h \
	svn_parser.h \
	bzr_parser.cpp \
	clients.cpp \
	cvs_parser.cpp \
	conditions.cpp \
	db.cpp \
	files.cpp \
	filters.cpp \
	interface.cpp \
	list.cpp \
	paths.cpp \
	strings.cpp \
	svn_parser.cpp

# Header file
library_includedir = $(includedir)
library_include_HEADERS = hbackup.h