# Makefile for wxd

CLDFLAGS = -g
CFLAGS = $(CLDFLAGS) -Wall -O2 -Idigitemp -I../../libmisc -I../../libastro
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS =  -lmisc -lfits -lastro -lm

DOBJS = digitemp/ds1820.o digitemp/onewire.o

OBJS = wxd.o davis.o page.o petebros.o rainwise.o digitemp.o url.o $(DOBJS)

wxd: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass4:	install

install: wxd
	cp wxd $(TELHOME)/bin

clobber:
	touch x.o
	rm -f *.o wxd

