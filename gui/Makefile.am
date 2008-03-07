SUBDIRS = .

bin_PROGRAMS = qhbackup
qhbackup_SOURCES = \
	choose-dialog.uih \
	qhbackup.cpp

LDADD = @top_srcdir@/lib/libhbackup.la -lQtGui
AM_CXXFLAGS = -I@top_srcdir@/lib -I@oldincludedir@/qt4/
#AM_LDFLAGS = -lssl -lz

clean-local:
	-rm -f choose-dialog.uih

# Rules
%.uih: %.ui
	uic -o $@ $<
