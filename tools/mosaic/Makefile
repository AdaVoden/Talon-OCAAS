
# Makefile for mosaic, pixarrange, and stampfits
# $Id: Makefile,v 1.1.1.1 2003/04/15 20:48:35 lostairman Exp $

CLDFLAGS = -ffast-math
CFLAGS = $(CLDFLAGS) -Wall -O2 -I../../libfits -I../../libmisc -I../../libastro -I../../libfs -I../../libwcs
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lfits -lmisc -lastro -lfs -lwcs -lm
DIR = $(TELHOME)/bin

ALL = mosaic stampfits

pass5:	install

install: $(ALL)
	cd $(DIR); rm -f $(ALL)
	cp $(ALL) $(DIR)
	touch x.o
	rm *.o

mosaic: mosaic.o rectangles.o
	$(CC) $(LDFLAGS) -o $@ mosaic.o rectangles.o $(LIBS)

stampfits: stampfits.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

clobber:
	touch x.o
	rm -f *.o $(ALL)

