AM_CXXFLAGS = \
  -I@top_srcdir@/include \
  -I@top_srcdir@/base \
  $(NULL)

LDADD = \
  @top_srcdir@/base/libhtools.la \
  @top_srcdir@/lib/libhbackup.la \
  $(NULL)

bin_PROGRAMS = hbackup

bin_SCRIPTS = hwbackup

hbackup_SOURCES = hbackup.cpp

install-exec-local:
	rm -f '$(DESTDIR)$(bindir)/hubackup'
	$(LN_S) hbackup '$(DESTDIR)$(bindir)/hubackup'

uninstall-local:
	rm -f '$(DESTDIR)$(bindir)/hubackup'

EXTRA_DIST = \
	hwbackup
