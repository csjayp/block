#
#
PREFIX?=/usr/local
CFLAGS+=	-pipe -O2 -Wall -g
OBJS=	block.c
LIBS=
CC?=	cc
TARGETS=	block

all: $(TARGETS)

block:	$(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS) $(CFLAGS)

install:
	cp block $(PREFIX)/bin
	cp block.1.gz $(PREFIX)/man/man1/

deinstall:
	rm -f $(PREFIX)/man/man1/block.1.gz
	rm -f $(PREFIX)/bin/block

clean:
	rm -f block block.1.gz
