include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I$(top_srcdir)/base/include \
  -I$(top_srcdir)/lib/include \
  -I$(top_srcdir)/lib/src \
  -I../src \
  $(NULL)

check_PROGRAMS = \
  data_client_test \
  test_server \
  $(NULL)

test_server_SOURCES = test_server.cpp
data_client_test_SOURCES = data_client_test.cpp

LDADD = \
  $(top_builddir)/base/src/libhtools.la \
  ../src/libhbackend_client.la \
  $(NULL)

check_SUCCESSES = \
  hbackend.done \
  client.done \
  $(NULL)

hbackend.done: ../src/hbackend
client.done: data_client_test test_server

EXTRA_DIST = \
  hbackend_test \
  hbackend.exp \
  client_test \
  client.exp \
  $(NULL)

