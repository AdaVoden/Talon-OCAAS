CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libastro
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lm

OBJS= dynamics.o

dynamics: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: dynamics
	rm -f $(TELHOME)/bin/dynamics
	cp dynamics $(TELHOME)/bin/dynamics

clobber:
	touch x.o
	rm -fr *.o dynamics

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:32 $ $Revision: 1.1.1.1 $ $Nam
