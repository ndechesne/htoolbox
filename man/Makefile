PREFIX ?= /usr/local

all:

install: hbackup.1
	@echo "INSTALL	$<"
	@mkdir -p ${PREFIX}/share/man/man1
	@cp $^ ${PREFIX}/share/man/man1
	@-rm -f ${PREFIX}/share/man/man1/$^.gz
	@-gzip -q ${PREFIX}/share/man/man1/$^

clean:

check: