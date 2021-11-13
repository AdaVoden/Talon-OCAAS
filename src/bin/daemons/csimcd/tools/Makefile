# Makefile for csimcd

CLDFLAGS = -O2 -ffast-math -Wall -g
CFLAGS = $(CLDFLAGS) -I../../../libmisc
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lm

read485: read485.o
	rm -f $@
	$(CC) $(LDFLAGS) -o $@ read485.o $(LIBS)

clobber:
	touch x.o
	rm -f *.o read485

