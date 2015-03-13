BIN_DIR=$(DESTDIR)/usr/bin
LIB_DIR=$(DESTDIR)/usr/lib

LIBDIR=../libs
IDIRS=$(LIBDIR)/cdb $(LIBDIR)/netacl $(LIBDIR)/libcidr/include
LDIRS=$(LIBDIR)/libcidr/src
LIBS=cidr

CC=gcc
CFLAGS=-Wall -Winline -O3 $(foreach i, $(IDIRS), -I$i) $(foreach l, $(LDIRS), -L$l)
LDLIBS=$(foreach l, $(LIBS), -l$l)

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -ggdb
endif

.PHONY: install clean uninstall recurse

all: dbfilter-cidr

$(LIBDIR)/libcidr/src/libcidr.so.0: recurse
	$(MAKE) -C $(LIBDIR)/libcidr

$(LIBDIR)/cdb/cdb.o: recurse
	$(MAKE) -C $(LIBDIR)/cdb cdb.o

$(LIBDIR)/netacl/netacl.o: recurse
	$(MAKE) -C $(LIBDIR)/netacl netacl.o

dbfilter-cidr: dbfilter-cidr.c $(LIBDIR)/cdb/cdb.o $(LIBDIR)/netacl/netacl.o $(LIBDIR)/libcidr/src/libcidr.so.0

install: dbfilter-cidr
	install -d $(BIN_DIR)
	install -m 0755 dbfilter-cidr $(BIN_DIR)/dbfilter-cidr
	install -d $(LIB_DIR)
	install -m 0644 $(LIBDIR)/libcidr/src/libcidr.so.0 $(LIB_DIR)/libcidr.so.0

clean:
	$(MAKE) -C $(LIBDIR)/cdb clean
	$(MAKE) -C $(LIBDIR)/netacl clean
	$(MAKE) -C $(LIBDIR)/libcidr clean
	rm -f dbfilter-cidr

uninstall:
	rm -f $(BIN_DIR)/dbfilter-cidr
	rm -f $(LIB_DIR)/libcidr.so.0

recurse:
	true
