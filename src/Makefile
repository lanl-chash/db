BIN_DIR=$(DESTDIR)/usr/bin
SHARE_DIR=$(DESTDIR)/usr/share/db
PYTHON_LIB_DIR=$(DESTDIR)/usr/share/pyshared
RSYNC_FLAGS=-arv --exclude .svn/ --exclude *.pyc

.PHONY: build install clean uninstall

all: build

build:
	$(MAKE) -C mux
	$(MAKE) -C dbfilter-cidr
	$(MAKE) -C dbsplit
	$(MAKE) -C timefind

install: build
	$(MAKE) -C mux install
	$(MAKE) -C dbfilter-cidr install
	$(MAKE) -C dbsplit install
	$(MAKE) -C timefind install
	install -d $(BIN_DIR)
	install -m 0755 dbcat $(BIN_DIR)/dbcat
	install -m 0755 dbsort $(BIN_DIR)/dbsort
	install -m 0755 dbsqawk $(BIN_DIR)/dbsqawk
	install -m 0755 dbcat $(BIN_DIR)/dbcat
	install -m 0755 db2sqlite $(BIN_DIR)/db2sqlite
	install -m 0755 db2json $(BIN_DIR)/db2json
	install -m 0755 dbstrip $(BIN_DIR)/dbstrip
	install -m 0755 jsoncat $(BIN_DIR)/jsoncat
	ln -f -s $(SHARE_DIR)/jsonsql $(BIN_DIR)/jsonsql
	ln -f -s $(SHARE_DIR)/jsonsort $(BIN_DIR)/jsonsort
	ln -f -s $(SHARE_DIR)/jsonsplit $(BIN_DIR)/jsonsplit
	install -d $(SHARE_DIR)
	install -m 0755 jsonsql $(SHARE_DIR)/jsonsql
	install -m 0755 jsonsort $(SHARE_DIR)/jsonsort
	install -m 0755 jsonsplit $(SHARE_DIR)/jsonsplit
	rsync $(RSYNC_FLAGS) node_modules $(SHARE_DIR)
	install -d $(SHARE_DIR)/bin
	install -m 0755 catmux $(SHARE_DIR)/bin/catmux
	install -d $(PYTHON_LIB_DIR)
	rsync $(RSYNC_FLAGS) libs/db $(PYTHON_LIB_DIR)
ifeq ($(DESTDIR),)
	ln -f -s $(PYTHON_LIB_DIR)/db $(DESTDIR)/usr/lib/python2.7/dist-packages/db
endif

clean:
	$(MAKE) -C mux clean
	$(MAKE) -C dbfilter-cidr clean
	$(MAKE) -C dbsplit clean
	$(MAKE) -C timefind clean

uninstall:
	$(MAKE) -C mux uninstall
	$(MAKE) -C dbfilter-cidr uninstall
	$(MAKE) -C dbsplit uninstall
	$(MAKE) -C timefind uninstall
	rm -f $(BIN_DIR)/dbsort
	rm -f $(BIN_DIR)/dbsqawk
	rm -f $(BIN_DIR)/dbcat
	rm -f $(BIN_DIR)/db2sqlite
	rm -f $(BIN_DIR)/db2json
	rm -f $(BIN_DIR)/dbstrip
	rm -f $(BIN_DIR)/jsoncat
	rm -f $(BIN_DIR)/jsonsql
	rm -f $(BIN_DIR)/jsonsort
	rm -f $(BIN_DIR)/jsonsplit
	rm -f $(SHARE_DIR)/jsonsql
	rm -f $(SHARE_DIR)/jsonsort
	rm -f $(SHARE_DIR)/jsonsplit
	rm -rf $(SHARE_DIR)/node_modules
	rm -f $(SHARE_DIR)/bin/catmux
	rmdir $(SHARE_DIR)/bin
	rmdir $(SHARE_DIR)
	rm -rf $(PYTHON_LIB_DIR)/db
	rm -f $(DESTDIR)/usr/lib/python2.7/dist-packages/db
