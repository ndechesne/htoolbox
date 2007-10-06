VERSION = 0:2:0

SUBDIRS = test

AM_CXXFLAGS = -Wall -ansi

# Library
lib_LTLIBRARIES = libhbackup.la
libhbackup_la_LDFLAGS= -version-info $(VERSION)

libhbackup_la_SOURCES = \
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
	clients.cpp \
	cvs_parser.cpp \
	conditions.cpp \
	db.cpp \
	files.cpp \
	filters.cpp \
	interface.cpp \
	list.cpp \
	paths.cpp \
	strings.cpp

# Header file
library_includedir = $(includedir)
library_include_HEADERS = hbackup.h