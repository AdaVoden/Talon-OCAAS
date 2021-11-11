CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libastro -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lmisc -lfits -lm

OBJS= shmd.o

shmd: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass4: install

install: shmd
	rm -f $(TELHOME)/bin/shmd
	cp shmd $(TELHOME)/bin/shmd

clobber:
	touch x.o
	rm -fr *.o shmd

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $
