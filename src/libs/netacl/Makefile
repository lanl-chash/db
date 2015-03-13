IDIRS=../libcidr/include

CC=gcc
CFLAGS=-Wall -Winline -Werror -march=native -O3 $(foreach i, $(IDIRS), -I$i)

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -ggdb
endif

.PHONY: clean

all: netacl.o

debug: CFLAGS += -DDEBUG

debug: clean netacl.o

netacl.o: netacl.c netacl.h

clean:
	rm -rf netacl.o
