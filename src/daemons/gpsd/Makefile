# Makefile for gpsd

CLDFLAGS =
CFLAGS = $(CLDFLAGS) -Wall -O2 -I../../libmisc -I../../libastro
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS =  -lastro -lmisc -lfits -lm

gpsd: gpsd.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

pass4:	install

install: gpsd
	cp gpsd $(TELHOME)/bin/gpsd
	cp fakegps $(TELHOME)/bin/fakegps

clobber:
	touch x.o
	rm -f *.o gpsd

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $
