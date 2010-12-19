include $(top_srcdir)/test_tools/test.mak

check_SUCCESSES = \
	hbackup.done \
	hwbackup.done

hbackup.done: ../src/hbackup
hwbackup.done: ../src/hbackup ../src/hwbackup
memleak.done: ../src/hbackup

EXTRA_DIST = \
	hbackup_test \
	hwbackup_test \
	hbackup.exp \
	hwbackup.exp
