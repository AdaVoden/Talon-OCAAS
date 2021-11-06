CFLAGS= -O2 -ffast-math -fPIC -Wall -I../libastro -I../libmisc $(MOTIFI)

OBJS =			\
	catbrowse.o 	\
	xtools.o

libXmisc.so:	$(OBJS)
	gcc -shared -o $@ $(OBJS)

pass2: install

install: libXmisc.so
	rm -f $(TELHOME)/lib/libXmisc.so
	cp libXmisc.so $(TELHOME)/lib/libXmisc.so

libXmisc.a:	$(OBJS)
	ar rv $@ $(OBJS)

clobber:
	touch x.o
	rm -f *.o libXmisc.a libXmisc.so

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $
