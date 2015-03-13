.PHONY: build deb tar clean uninstall

all: build

build:
	make -C src

install:
	make -C src install
ifeq ($(DESTDIR),)
	install -m 644 man/db2json.1 /usr/local/share/man/man1/db2json.1
	install -m 644 man/db2sqlite.1 /usr/local/share/man/man1/db2sqlite.1
	install -m 644 man/dbcat.1 /usr/local/share/man/man1/dbcat.1
	install -m 644 man/dbfilter-cidr.1 /usr/local/share/man/man1/dbfilter-cidr.1
	install -m 644 man/dbsort.1 /usr/local/share/man/man1/dbsort.1
	install -m 644 man/dbsplit.1 /usr/local/share/man/man1/dbsplit.1
	install -m 644 man/dbsqawk.1 /usr/local/share/man/man1/dbsqawk.1
	install -m 644 man/dbstrip.1 /usr/local/share/man/man1/dbstrip.1
	install -m 644 man/jsoncat.1 /usr/local/share/man/man1/jsoncat.1
	install -m 644 man/jsonsort.1 /usr/local/share/man/man1/jsonsort.1
	install -m 644 man/jsonsplit.1 /usr/local/share/man/man1/jsonsplit.1
	install -m 644 man/jsonsql.1 /usr/local/share/man/man1/jsonsql.1
	mandb
endif

deb:
	dpkg-buildpackage -I -us -uc -tc

tar: clean
	cd .. && tar hcfz db-0.2.tar.gz --exclude-vcs db-0.2

clean:
	make -C src clean

uninstall:
	make -C src uninstall
	rm -f /usr/local/share/man/man1/db2json.1
	rm -f /usr/local/share/man/man1/db2sqlite.1
	rm -f /usr/local/share/man/man1/dbcat.1
	rm -f /usr/local/share/man/man1/dbfilter-cidr.1
	rm -f /usr/local/share/man/man1/dbsort.1
	rm -f /usr/local/share/man/man1/dbsplit.1
	rm -f /usr/local/share/man/man1/dbsqawk.1
	rm -f /usr/local/share/man/man1/dbstrip.1
	rm -f /usr/local/share/man/man1/jsoncat.1
	rm -f /usr/local/share/man/man1/jsonsort.1
	rm -f /usr/local/share/man/man1/jsonsplit.1
	rm -f /usr/local/share/man/man1/jsonsql.1
	mandb
