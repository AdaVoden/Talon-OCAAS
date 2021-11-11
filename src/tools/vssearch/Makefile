CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math -Wall -I../../libastro  -I../../libwcs -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lmisc -lfits -lastro -lwcs -lfs -lm

OBJS= vssearch.o

vssearch: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: vssearch
	rm -f $(TELHOME)/bin/vssearch
	cp vssearch $(TELHOME)/bin/vssearch

pass5: install

clobber:
	touch x.o
	rm -fr *.o vssearch

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:46 $ $Revision: 1.1.1.1 $ $Name:  $
