CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -Wall
LDFLAGS= $(CLDFLAGS) -s
LIBS = -lm

OBJS= csi.o

csi: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: csi
	rm -f $(TELHOME)/bin/csi
	cp csi $(TELHOME)/bin/csi

clobber:
	touch x.o
	rm -fr *.o csi

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:32 $ $Revision: 1.1.1.1 $ $Name:  $
