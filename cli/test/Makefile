all:
install:

check:	hbackup.done

clean:
	@rm -f *.[oa] *~ *.all *.done
	@../../test_tools/test_setup clean

# Rules
hbackup.done: ../hbackup hbackup_test hbackup.exp
	@echo "RUN	$<"
	@./hbackup_test `basename $@ .done` && touch $@
