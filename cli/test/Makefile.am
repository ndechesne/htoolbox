check_SUCCESSES = \
	hbackup.done

check-local: $(check_SUCCESSES)

CLEANFILES = $(check_SUCCESSES)

clean-local:
	-rm -rf *test_dir

hbackup.done: hbackup_test ../hbackup hbackup.exp @top_srcdir@/$(TEST_RUN) test_dir
	@echo "RUN	$<"
	@./hbackup_test `basename $@ .done` && touch $@

# Rules
test_dir: @top_srcdir@/$(TEST_TOOL)
	$< $@

%.done: %_test %.exp @top_srcdir@/$(TEST_RUN) test_dir
	@echo "RUN	$<"
	@@top_srcdir@/$(TEST_RUN) `basename $@ .done` && touch $@
