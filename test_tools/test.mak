check-local: $(check_SUCCESSES)

CLEANFILES = $(check_SUCCESSES)

clean-local:
	-rm -rf *test_dir*

# Rules
test_dir.tar.gz: $(top_srcdir)/$(TEST_TOOL)
	$< $(subst .tar.gz,,$(@))

%.done: %_test %.exp $(top_srcdir)/$(TEST_RUN) test_dir.tar.gz
	$(top_srcdir)/$(TEST_RUN) $(subst .done,,$(@))
