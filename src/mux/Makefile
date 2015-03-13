SHARE_DIR=$(DESTDIR)/usr/share/db

CC=gcc
CFLAGS=-Wall -Werror -ansi -pedantic

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -ggdb
endif

.PHONY: clean install uninstall

all: mux

mux: mux.c

install: mux
	install -d $(SHARE_DIR)/bin
	install -m 0755 mux $(SHARE_DIR)/bin/mux

clean:
	rm -f mux

uninstall:
	rm -f $(SHARE_DIR)/bin/mux
	rmdir --ignore-fail-on-non-empty $(SHARE_DIR)/bin
	rmdir --ignore-fail-on-non-empty $(SHARE_DIR)
