# Makefile for fitshdr

CLDFLAGS =
CFLAGS = $(CLDFLAGS) -Wall -O2 -I../../libfits -I../../libmisc -I../../libastro
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS =  -lfits -lmisc -lastro -lm

all: fitshdr fitsvalue

fitshdr: fitshdr.o 
	$(CC) $(LDFLAGS) -o $@ fitshdr.o $(LIBS)

fitsvalue: fitsvalue.o 
	$(CC) $(LDFLAGS) -o $@ fitsvalue.o $(LIBS)

pass5:	install

install: all 
	cp fitshdr $(TELHOME)/bin
	cp fitsvalue $(TELHOME)/bin

clobber:
	touch x.o
	rm -f *.o fitshdr fitsvalue

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:33 $ $Revision: 1.1.1.1 $ $Name:  $
