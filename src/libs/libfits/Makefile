CFLAGS= -O2 -ffast-math -fPIC -Wall -I../libmisc -I../libastro -I../libwcs

OBJS =	\
	align2fits.o	\
	fits.o		\
	fitscorr.o	\
	filters.o	\
	fitsip.o

libfits.so:	$(OBJS)
	gcc -shared -o $@ $(OBJS)

pass2: install

install: libfits.so
	rm -f $(TELHOME)/lib/libfits.so
	cp libfits.so $(TELHOME)/lib/libfits.so


libfits.a:	$(OBJS)
	ar rv $@ $(OBJS)


bzero:
	make -f Makefile.bzero clobber
	make -f Makefile.bzero install	
	

pass2jsf-halfm:	bzero
pass2jsf-onem:	bzero

clobber:
	touch x.o
	rm -f *.o libfits.a libfits.so

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:14 $ $Revision: 1.1.1.1 $ $Name:  $
