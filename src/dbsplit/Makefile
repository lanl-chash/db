BIN_DIR=$(DESTDIR)/usr/bin

LIBDIR=../libs
IDIRS=$(LIBDIR)/cdb

CC=gcc
CFLAGS=-Wall -O3 $(foreach i, $(IDIRS), -I$i)

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -ggdb
endif

.PHONY: install clean uninstall recuruse

all: dbsplit

xxhash.o: xxhash.c xxhash.h

$(LIBDIR)/cdb/cdb.o: recurse
	$(MAKE) -C $(LIBDIR)/cdb cdb.o

dbsplit: dbsplit.c $(LIBDIR)/cdb/cdb.o xxhash.o

install: dbsplit
	install -d $(BIN_DIR)
	install -m 0755 dbsplit $(BIN_DIR)/dbsplit

clean:
	$(MAKE) -C $(LIBDIR)/cdb clean
	rm -f dbsplit *.o

uninstall:
	rm -f $(BIN_DIR)/dbsplit

recurse:
	true
