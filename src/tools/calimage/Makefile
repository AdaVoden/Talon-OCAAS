CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math -I../../libastro -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lm

OBJS= calimage.o

calimage: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: calimage
	rm -f $(TELHOME)/bin/calimage
	cp calimage $(TELHOME)/bin/calimage

pass5: install

clobber:
	touch x.o
	rm -fr *.o calimage

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:32 $ $Revision: 1.1.1.1 $ $Name:  $
