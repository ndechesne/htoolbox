all:
	@$(MAKE) -C lib $@
	@$(MAKE) -C cli $@
	@$(MAKE) -C man $@

install:
	@$(MAKE) -C lib $@
	@$(MAKE) -C cli $@
	@$(MAKE) -C man $@

clean:
	@$(MAKE) -C lib $@
	@$(MAKE) -C cli $@
	@$(MAKE) -C man $@

check:
	@$(MAKE) -C lib $@
	@$(MAKE) -C cli $@
	@$(MAKE) -C man $@
