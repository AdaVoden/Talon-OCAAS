CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libwcs -I../../libfs -I../../libastro -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lmisc -lfits -lwcs -lfs -lm

OBJS= orbfit.o

orbfit: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: orbfit
	rm -f $(TELHOME)/bin/orbfit
	cp orbfit $(TELHOME)/bin/orbfit

clobber:
	touch x.o
	rm -fr *.o orbfit

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:35 $ $Revision: 1.1.1.1 $ $Name:  $
