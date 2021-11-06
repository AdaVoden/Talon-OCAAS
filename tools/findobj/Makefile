IS = 	-I../../libastro \
	-I../../libwcs \
	-I../../libfits \
	-I../../libmisc

CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math $(IS)
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lwcs -lmisc -lfits -lfs -lm

OBJS= findobj.o

findobj: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: findobj
	rm -f $(TELHOME)/bin/findobj
	cp findobj $(TELHOME)/bin/findobj

clobber:
	touch x.o
	rm -f *.o findobj
	rm -f *~

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:33 $ $Revision: 1.1.1.1 $ $Name:  $
