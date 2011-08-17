include $(top_srcdir)/test_tools/test.mak

AM_CXXFLAGS = \
  -I../include \
  $(NULL)

check_PROGRAMS = \
  abstract_socket_test \
  asyncwriter_test \
  configuration_test \
  copier_test \
  criticality_test \
  filereaderwriter_test \
  files_test \
  hash_tree_test \
  hasher_test \
  inet_socket_test \
  linereaderwriter_test \
  observer_test \
  process_mutex_test \
  report_test \
  shared_path_test \
  tlv_helper_test \
  tlv_test \
  unix_socket_test \
  threads_manager_test \
  threads_manager_extensive_test \
  zipper_test \
  $(NULL)

abstract_socket_test_SOURCES = abstract_socket_test.cpp
asyncwriter_test_SOURCES = asyncwriter_test.cpp
configuration_test_SOURCES = configuration_test.cpp
copier_test_SOURCES = copier_test.cpp
criticality_test_SOURCES = criticality_test.cpp
filereaderwriter_test_SOURCES = filereaderwriter_test.cpp
files_test_SOURCES = files_test.cpp
hash_tree_test_SOURCES = hash_tree_test.cpp
hasher_test_SOURCES = hasher_test.cpp
inet_socket_test_SOURCES = inet_socket_test.cpp
linereaderwriter_test_SOURCES = linereaderwriter_test.cpp
observer_test_SOURCES = observer_test.cpp
process_mutex_test_SOURCES = process_mutex_test.cpp
report_test_SOURCES = report_test.cpp
shared_path_test_SOURCES = shared_path_test.cpp
tlv_helper_test_SOURCES = tlv_helper_test.cpp
tlv_test_SOURCES = tlv_test.cpp
unix_socket_test_SOURCES = unix_socket_test.cpp
threads_manager_test_SOURCES = threads_manager_test.cpp
threads_manager_extensive_test_SOURCES = threads_manager_extensive_test.cpp
zipper_test_SOURCES = zipper_test.cpp

abstract_socket_test.cpp: socket_test.cpp Makefile
	cat $< \
	| sed "s/@@FIRST@@/\"this_is_quite_a_long_socket\", true/" \
	| sed "s/@@SECOND@@/\"this_is_quite_a_long_socket\", true/" \
	| sed "s/@@THIRD@@/\"this_is_quite_a_long_socket\", true/" \
	> $@

inet_socket_test.cpp: socket_test.cpp Makefile
	cat $< \
	| sed "s/@@FIRST@@/\"0.0.0.0\", 12345/" \
	| sed "s/@@SECOND@@/\"localhost\", 12345/" \
	| sed "s/@@THIRD@@/\"127.0.0.1\", 12345/" \
	> $@

unix_socket_test.cpp: socket_test.cpp Makefile
	cat $< \
	| sed "s/@@FIRST@@/\"this_is_quite_a_long_socket\"/" \
	| sed "s/@@SECOND@@/\"this_is_quite_a_long_socket\"/" \
	| sed "s/@@THIRD@@/\"this_is_quite_a_long_socket\"/" \
	> $@

LDADD = ../src/libhtoolbox.la

abstract_socket.done: abstract_socket_test.cpp
inet_socket.done: inet_socket_test.cpp
unix_socket.done: unix_socket_test.cpp

check_SUCCESSES = \
  hash_tree.done \
  shared_path.done \
  observer.done \
  configuration.done \
  filereaderwriter.done \
  inet_socket.done \
  unix_socket.done \
  abstract_socket.done \
  process_mutex.done \
  tlv.done \
  tlv_helper.done \
  linereaderwriter.done \
  hasher.done \
  asyncwriter.done \
  copier.done \
  files.done \
  criticality.done \
  report.done \
  zipper.done \
  $(NULL)

EXTRA_DIST = \
  abstract_socket.exp \
  asyncwriter.exp \
  configuration.exp \
  criticality.exp \
  files.exp \
  filereaderwriter.exp \
  hash_tree.exp \
  hasher.exp \
  inet_socket.exp \
  linereaderwriter.exp \
  observer.exp \
  process_mutex.exp \
  report.exp \
  shared_path.exp \
  tlv_helper.exp \
  tlv.exp \
  unix_socket.exp \
  threads_manager.exp \
  threads_manager_extensive.exp \
  zipper.exp \
  $(NULL)
