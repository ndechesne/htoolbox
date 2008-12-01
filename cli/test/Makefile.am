include $(top_srcdir)/test_tools/test.mak

check_SUCCESSES = \
	hbackup.done \
	hwbackup.done

hbackup.done: ../hbackup
hwbackup.done: ../hbackup ../hwbackup
memleak.done: ../hbackup

EXTRA_DIST = \
	hbackup_test \
	hwbackup_test \
	hbackup.exp \
	hwbackup.exp
