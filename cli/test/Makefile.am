check_SUCCESSES = \
	hbackup.done

check-local: $(check_SUCCESSES)

CLEANFILES = $(check_SUCCESSES)

clean-local:
	-rm -rf *test_dir

hbackup.done: hbackup_test ../hbackup hbackup.exp @top_srcdir@/$(TEST_RUN) test_dir
	./hbackup_test $(subst .done,,$(@))

# Rules
test_dir: @top_srcdir@/$(TEST_TOOL)
	$< $@

%.done: %_test %.exp @top_srcdir@/$(TEST_RUN) test_dir
	@top_srcdir@/$(TEST_RUN) $(subst .done,,$(@))
