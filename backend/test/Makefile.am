include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I../src \
  $(NULL)

check_PROGRAMS = \
  receiver_test \
  sender_test \
  $(NULL)

receiver_test_SOURCES = receiver_test.cpp ../src/protocol.cpp
sender_test_SOURCES = sender_test.cpp ../src/protocol.cpp

LDADD = \
  $(top_builddir)/base/src/libhtools.la \
  $(NULL)

check_SUCCESSES = \
  hbackend.done \
  protocol.done \
  $(NULL)

hbackend.done: ../src/hbackend
protocol.done: receiver_test sender_test protocol_test

EXTRA_DIST = \
  hbackend_test \
  hbackend.exp \
  protocol_test \
  protocol.exp \
  $(NULL)

