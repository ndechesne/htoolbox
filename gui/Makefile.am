SUBDIRS = .

bin_PROGRAMS = qhbackup
qhbackup_SOURCES = \
	choose-dialog.uih \
	moc_qhbackup.cpp \
	qhbackup.cpp

LDADD = @top_srcdir@/lib/libhbackup.la -lQtGui
AM_CXXFLAGS = -I@top_srcdir@/lib -I@oldincludedir@/qt4/

clean-local:
	-rm -f choose-dialog.uih

# Rules
%.uih: %.ui
	uic -o $@ $<

moc_%.cpp: %.h
	moc $< -o $@