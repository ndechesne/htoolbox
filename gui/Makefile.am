SUBDIRS = .

bin_PROGRAMS = qhbackup
qhbackup_SOURCES = \
	choose-dialog.uih \
	choose.h \
	check.h \
	moc_choose.cpp \
	moc_check.cpp \
	choose.cpp \
	check.cpp \
	qhbackup.cpp

LDADD = @top_srcdir@/lib/libhbackup.la -lQtGui
AM_CXXFLAGS = -I@top_srcdir@/lib -I@oldincludedir@/qt4/

clean-local:
	-rm -f *.uih moc_*.cpp

# Rules
%.uih: %.ui
	uic -o $@ $<

moc_%.cpp: %.h choose-dialog.uih
	moc $< -o $@
