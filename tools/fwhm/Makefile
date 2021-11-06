# Makefile for fwhm

CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math -I../../libfits -I../../libmisc -I../../libastro 
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
DIR = $(TELHOME)/bin
LIBS = -lastro -lmisc -lfits -lm

OBJS= fwhm.o

fwhm: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5:	install

install: fwhm
	rm -f $(TELHOME)/bin/fwhm
	cp fwhm $(TELHOME)/bin/fwhm

clobber:
	touch x.o
	rm -f *.o fwhm

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $
