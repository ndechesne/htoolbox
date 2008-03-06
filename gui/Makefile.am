SUBDIRS = .

bin_PROGRAMS = qhbackup
qhbackup_SOURCES = \
	qhbackup.cpp \
	choose-dialog.ui

LDADD = @top_srcdir@/lib/libhbackup.la -lQtGui
AM_CXXFLAGS = -I@top_srcdir@/lib -I@oldincludedir@/qt4/
AM_LDFLAGS = -lssl -lz

# Rules
%.uih: %.ui
	uic -o $@ $<
