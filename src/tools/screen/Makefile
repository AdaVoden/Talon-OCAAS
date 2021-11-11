CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall
LDFLAGS= $(CLDFLAGS)
LIBS = -lncurses -ltermcap

OBJS= screen.o

screen: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: screen
	rm -f $(TELHOME)/bin/screen
	cp screen $(TELHOME)/bin/screen

clobber:
	touch x.o
	rm -fr *.o screen

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:39 $ $Revision: 1.1.1.1 $ $Name:  $
