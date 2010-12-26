include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I$(top_srcdir)/base/include \
  -I$(top_srcdir)/lib/include \
  -I$(top_srcdir)/lib/src \
  -I../src \
  $(NULL)

check_PROGRAMS = \
  receiver_test \
  sender_test \
  data_client_test \
  test_server \
  $(NULL)

receiver_test_SOURCES = receiver_test.cpp
sender_test_SOURCES = sender_test.cpp
test_server_SOURCES = test_server.cpp
data_client_test_SOURCES = data_client_test.cpp

LDADD = \
  $(top_builddir)/base/src/libhtools.la \
  ../src/libhbackend_client.la \
  $(NULL)

check_SUCCESSES = \
  hbackend.done \
  protocol.done \
  $(NULL)

hbackend.done: ../src/hbackend
protocol.done: receiver_test sender_test protocol_test
client.done: data_client_test test_server

EXTRA_DIST = \
  hbackend_test \
  hbackend.exp \
  protocol_test \
  protocol.exp \
  client_test \
  client.exp \
  $(NULL)

