CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libwcs -I../../libfs -I../../libastro -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lmisc -lfits -lwcs -lfs -lm

OBJS= fitsfilter.o

fitsfilter: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: fitsfilter
	rm -f $(TELHOME)/bin/fitsfilter
	cp fitsfilter $(TELHOME)/bin/fitsfilter

clobber:
	touch x.o
	rm -fr *.o fitsfilter

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:33 $ $Revision: 1.1.1.1 $ $Name:  $
