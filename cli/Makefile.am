SUBDIRS = . test man

bin_PROGRAMS = hbackup
hbackup_SOURCES = hbackup.cpp
LDADD = @top_srcdir@/lib/libhbackup.la
AM_CXXFLAGS = -I@top_srcdir@/lib

install-exec-local:
	rm -f '$(DESTDIR)$(bindir)/hubackup'
	$(LN_S) hbackup '$(DESTDIR)$(bindir)/hubackup'
