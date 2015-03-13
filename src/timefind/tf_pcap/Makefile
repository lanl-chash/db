BIN_DIR=$(DESTDIR)/usr/bin

LIBS=pcap

CC=gcc
CFLAGS=-Wall -Winline -O3
LDLIBS=$(foreach l, $(LIBS), -l$l)

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -ggdb
endif

.PHONY: clean install uninstall

all: tf_pcap

tf_pcap: tf_pcap.c

install: tf_pcap tf_pcapz
	install -d $(BIN_DIR)
	install -m 0755 tf_pcap $(BIN_DIR)/tf_pcap
	install -m 0755 tf_pcapz $(BIN_DIR)/tf_pcapz

clean:
	rm -f tf_pcap

uninstall:
	rm -f $(BIN_DIR)/tf_pcap
	rm -f $(BIN_DIR)/tf_pcapz
