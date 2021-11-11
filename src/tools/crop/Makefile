# Makefile for crop

CLDFLAGS= 
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libastro -I../../libfits -I../../libwcs -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib $(MOTIFL)
LIBS= -lastro -lfits -lmisc -lwcs -lfs -lm -lX11

OBJS= crop.o

crop: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5:	install

install: crop
	rm -f $(TELHOME)/bin/crop
	cp crop $(TELHOME)/bin/crop

clobber:
	touch x.o
	rm -f *.o crop

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:32 $ $Revision: 1.1.1.1 $ $Name:  $
