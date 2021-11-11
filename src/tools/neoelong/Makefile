CLDFLAGS= 
CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math -I../../libastro -I../../libwcs -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lmisc -lfits -lwcs -lfs -lm

OBJS= neoelong.o

neoelong: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: neoelong
	rm -f $(TELHOME)/bin/neoelong
	cp neoelong $(TELHOME)/bin/neoelong

clobber:
	touch x.o
	rm -fr *.o neoelong

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:35 $ $Revision: 1.1.1.1 $ $Name:  $
