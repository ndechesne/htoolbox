SUBDIRS = . test man

AM_CXXFLAGS = \
  -I@top_srcdir@/include \
  $(NULL)

LDADD = @top_srcdir@/lib/libhbackup.la

bin_PROGRAMS = hbackup

hbackup_SOURCES = hbackup.cpp

install-exec-local:
	rm -f '$(DESTDIR)$(bindir)/hubackup'
	$(LN_S) hbackup '$(DESTDIR)$(bindir)/hubackup'
	install -c hwbackup '$(DESTDIR)$(bindir)/hwbackup'

uninstall-local:
	rm -f '$(DESTDIR)$(bindir)/hubackup'
	rm -f '$(DESTDIR)$(bindir)/hwbackup'

EXTRA_DIST = \
	hwbackup
