include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I../include \
  $(NULL)

check_PROGRAMS = \
  asyncwriter_test \
  configuration_test \
  filereaderwriter_test \
  files_test \
  hasher_test \
  inet_socket_test \
  linereaderwriter_test \
  report_test \
  unix_socket_test \
  work_scheduler_test \
  work_scheduler_extensive_test \
  zip_unzip_test \
  $(NULL)

asyncwriter_test_SOURCES = asyncwriter_test.cpp
configuration_test_SOURCES = configuration_test.cpp
filereaderwriter_test_SOURCES = filereaderwriter_test.cpp
files_test_SOURCES = files_test.cpp
hasher_test_SOURCES = hasher_test.cpp
inet_socket_test_SOURCES = inet_socket_test.cpp
linereaderwriter_test_SOURCES = linereaderwriter_test.cpp
report_test_SOURCES = report_test.cpp
unix_socket_test_SOURCES = unix_socket_test.cpp
work_scheduler_test_SOURCES = work_scheduler_test.cpp
work_scheduler_extensive_test_SOURCES = work_scheduler_extensive_test.cpp
zip_unzip_test_SOURCES = zip_unzip_test.cpp

inet_socket_test.cpp: socket_test.cpp
	cat $< \
	| sed "s/@@FIRST@@/\"0.0.0.0\", 12345/" \
	| sed "s/@@SECOND@@/\"localhost\", 12345/" \
	| sed "s/@@THIRD@@/\"127.0.0.1\", 12345/" \
	> $@

unix_socket_test.cpp: socket_test.cpp
	cat $< \
	| sed "s/@@FIRST@@/\"socket\"/" \
	| sed "s/@@SECOND@@/\"socket\"/" \
	| sed "s/@@THIRD@@/\"socket\"/" \
	> $@

LDADD = ../src/libhtools.la

check_SUCCESSES = \
  configuration.done \
  filereaderwriter.done \
  inet_socket.done \
  unix_socket.done \
  linereaderwriter.done \
  hasher.done \
  asyncwriter.done \
  files.done \
  report.done \
  zip_unzip.done \
  $(NULL)

EXTRA_DIST = \
  asyncwriter.exp \
  configuration.exp \
  files.exp \
  filereaderwriter.exp \
  hasher.exp \
  inet_socket.exp \
  linereaderwriter.exp \
  report.exp \
  unix_socket.exp \
  work_scheduler.exp \
  work_scheduler_extensive.exp \
  zip_unzip.exp \
  $(NULL)
