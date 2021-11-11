CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -fPIC -Wall -I../libmisc -I../libastro
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
SLIBS = -lfs -lfits -lmisc -lastro -lm

LOBJS = \
	gsc.o \
	usno.o

libfs.so: $(LOBJS)
	gcc -shared -o $@ $(LOBJS)

pass2: install

install: libfs.so
	rm -f $(TELHOME)/lib/libfs.so
	cp libfs.so $(TELHOME)/lib/libfs.so


SOBJS = sadump.o

sadump: $(SOBJS)
	$(CC) $(LDFLAGS) -o $@ $(SOBJS) $(SLIBS)

libfs.a: $(LOBJS)
	ar rv $@ $(LOBJS)


clobber:
	touch x.o
	rm -f *.o libfs.a libfs.so sadump

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:17 $ $Revision: 1.1.1.1 $ $Name:  $
