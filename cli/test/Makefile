TEST_TOOL = ../../test_tools/test_setup
TEST_RUN = ../../test_tools/test_run

all:
install:

check:	hbackup.done

clean:
	@rm -f *.[oa] *~ *.all *.done
	@$(TEST_TOOL) clean

# Rules
hbackup.done: ../hbackup hbackup_test hbackup.exp $(TEST_RUN) $(TEST_TOOL)
	@echo "RUN	$<"
	@./hbackup_test `basename $@ .done` && touch $@
