CLDFLAGS =
CFLAGS = $(CLDFLAGS) -Wall -O2
LDFLAGS = $(CLDFLAGS)

bdm12: bdm12.o
	cc $(LDFLAGS) -o $@ $?

pass5: install

install: bdm12
	rm -f $(TELHOME)/bin/bdm12
	mv bdm12 $(TELHOME)/bin/bdm12
	
clobber:
	touch x.o
	rm -f *.o bdm12


# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:46:47 $ $Revision: 1.1.1.1 $ $Name:  $
