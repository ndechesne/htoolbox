check_SUCCESSES = \
	hbackup.done

check-local: $(check_SUCCESSES)

CLEANFILES = $(check_SUCCESSES)

clean-local:
	-rm -rf *test_dir

# Rules
test_dir: @top_srcdir@/$(TEST_TOOL)
	$< $@

%.done: %_test %.exp @top_srcdir@/$(TEST_RUN) test_dir
	@top_srcdir@/$(TEST_RUN) $(subst .done,,$(@))
