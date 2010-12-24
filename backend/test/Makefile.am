include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I../src \
  $(NULL)

check_PROGRAMS = \
  protocol_test \
  $(NULL)

protocol_test_SOURCES = protocol_test.cpp ../src/protocol.cpp

LDADD = \
  $(top_builddir)/base/src/libhtools.la \
  $(NULL)

check_SUCCESSES = \
  hbackend.done \
  protocol.done \
  $(NULL)

hbackend.done: ../src/hbackend

EXTRA_DIST = \
  hbackend_test \
  hbackend.exp \
  $(NULL)

