IS = 	-I../../libastro \
	-I../../libwcs \
	-I../../libfits \
	-I../../libmisc

CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math $(IS)
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lwcs -lmisc -lfits -lfs -lm

OBJS= findstars.o

findstars: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: findstars
	rm -f $(TELHOME)/bin/findstars
	cp findstars $(TELHOME)/bin/findstars

clobber:
	touch x.o
	rm -f *.o findstars

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:33 $ $Revision: 1.1.1.1 $ $Name:  $
