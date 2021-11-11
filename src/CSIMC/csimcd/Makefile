# Makefile for csimcd

CLDFLAGS = -O2 -ffast-math -Wall -g
CFLAGS = $(CLDFLAGS) -I../../libmisc
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lm

OBJS =	csimcd.o

csimcd: $(OBJS)
	rm -f $@
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: csimcd
	rm -f $(TELHOME)/bin/csimcd
	cp csimcd $(TELHOME)/bin

pass4:	install

clobber:
	touch x.o
	rm -f *.o csimcd

