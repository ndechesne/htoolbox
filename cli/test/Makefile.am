check_SUCCESSES = \
	hbackup.done

check-local: $(check_SUCCESSES)

CLEANFILES = $(check_SUCCESSES)

clean-local:
	@echo "Calling test tool to clean test environment"
	@@top_srcdir@/$(TEST_TOOL) clean

hbackup.done: ../hbackup hbackup_test hbackup.exp @top_srcdir@/$(TEST_RUN) @top_srcdir@/$(TEST_TOOL)
	@echo "RUN	$<"
	@./hbackup_test `basename $@ .done` && touch $@

# Rules
%.done: %_test %.exp @top_srcdir@/$(TEST_RUN) @top_srcdir@/$(TEST_TOOL)
	@echo "RUN	$<"
	@@top_srcdir@/$(TEST_RUN) `basename $@ .done` && touch $@
