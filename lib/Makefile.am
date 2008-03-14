SUBDIRS = . test

# Library
lib_LTLIBRARIES = libhbackup.la
libhbackup_la_LDFLAGS= -version-info $(LIB_VERSION)

libhbackup_la_SOURCES = \
	clients.h \
	conditions.h \
	configuration.h \
	cvs_parser.h \
	data.h \
	db.h \
	files.h \
	filters.h \
	hbackup.h \
	list.h \
	missing.h \
	owner.h \
	parsers.h \
	paths.h \
	report.h \
	svn_parser.h \
	clients.cpp \
	conditions.cpp \
	configuration.cpp \
	cvs_parser.cpp \
	data.cpp \
	db.cpp \
	files.cpp \
	filters.cpp \
	interface.cpp \
	list.cpp \
	missing.cpp \
	owner.cpp \
	parsers.cpp \
	paths.cpp \
	report.cpp \
	svn_parser.cpp

# Header file
library_includedir = $(includedir)
library_include_HEADERS = hbackup.h
