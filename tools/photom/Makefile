# Makefile for photom

CLDFLAGS= 
CFLAGS= $(CLDFLAGS) -O2 -Wall -I../../libastro  -I../../libwcs -I../../libmisc -I../../libfits
LDFLAGS= $(CLDFLAGS)  -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lwcs -lfs -lm

OBJS= cropping.o doimages.o photom.o formats.o readconfig.o

photom: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5:	install

install: photom
	rm -f $(TELHOME)/bin/photom
	cp photom $(TELHOME)/bin/photom

clobber:
	touch x.o
	rm -f *.o photom

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:36 $ $Revision: 1.1.1.1 $ $Name:  $
