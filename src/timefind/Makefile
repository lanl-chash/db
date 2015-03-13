BIN_DIR=$(DESTDIR)/usr/bin
CONF_DIR=$(DESTDIR)/etc/db

.PHONY: build install clean uninstall

all: build

build:
	$(MAKE) -C tf_pcap

install: build
	$(MAKE) -C tf_pcap install
	install -d $(BIN_DIR)
	install -m 0755 timefind $(BIN_DIR)/timefind
	install -d $(CONF_DIR)
	install -m 0644 timefind.conf $(CONF_DIR)/timefind.conf
	install -m 0644 timefind.spec $(CONF_DIR)/timefind.spec

clean:
	$(MAKE) -C tf_pcap clean

uninstall:
	$(MAKE) -C tf_pcap uninstall
	rm -f $(BIN_DIR)/timefind
	rm -f $(CONF_DIR)/timefind.conf
	rm -f $(CONF_DIR)/timefind.spec
	rmdir --ignore-fail-on-non-empty $(CONF_DIR)
