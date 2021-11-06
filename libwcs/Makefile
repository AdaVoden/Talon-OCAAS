CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -fPIC -Wall -ffast-math -I../libmisc -I../libfits -I../libastro -I../libfs
LDFLAGS= $(CLDFLAGS) 

OBJS =	align2wcs.o delwcsfits.o setwcsfits.o checkwcsfits.o findreg.o findregd.o \
	worldpos.o xyradec.o

libwcs.so:	$(OBJS)
	gcc -shared -o $@ $(OBJS)

pass2: install

install: libwcs.so
	rm -f $(TELHOME)/lib/libwcs.so
	cp libwcs.so $(TELHOME)/lib/libwcs.so

libwcs.a:	$(OBJS)
	ar rv $@ $(OBJS)

clobber:
	touch x.o
	rm -f *.o libwcs.a libwcs.so

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:18 $ $Revision: 1.1.1.1 $ $Name:  $
