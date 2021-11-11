CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libwcs -I../../libfs -I../../libastro -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lmisc -lfits -lwcs -lfs -lm

OBJS= wcs.o

wcs: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: wcs
	rm -f $(TELHOME)/bin/wcs
	cp wcs $(TELHOME)/bin/wcs

clobber:
	touch x.o
	rm -fr *.o wcs

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:46 $ $Revision: 1.1.1.1 $ $Name:  $
