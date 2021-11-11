# Makefile for rund

CLDFLAGS = -O2 -ffast-math -Wall
CFLAGS = $(CLDFLAGS) -I../../libmisc
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lmisc -lastro -lfits -lm

OBJS =	rund.o

rund: $(OBJS)
	rm -f $@
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: rund
	rm -f $(TELHOME)/bin/rund
	cp rund $(TELHOME)/bin

pass4:	install

clobber:
	touch x.o
	rm -f *.o rund

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $
