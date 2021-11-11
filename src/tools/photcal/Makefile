# Makefile photcal

CLDFLAGS= 
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libastro  -I../../libwcs -I../../libmisc -I../../libfits
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lfits -lmisc -lwcs -lfs -lm

OBJS= photcal.o scandir.o solvev0kp.o

photcal: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: photcal
	rm -f $(TELHOME)/bin/photcal
	cp photcal $(TELHOME)/bin/photcal
	rm -f $(TELHOME)/archive/photcal/photcal.ref
	cp photcal.ref $(TELHOME)/archive/photcal
	rm -f $(TELHOME)/archive/photcal/photcal.out
	cp photcal.out $(TELHOME)/archive/photcal
	rm -f $(TELHOME)/archive/photcal/photcal.def
	cp photcal.def $(TELHOME)/archive/photcal

pass5:	install

clobber:
	touch x.o
	rm -f *.o photcal

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:35 $ $Revision: 1.1.1.1 $ $Name:  $
