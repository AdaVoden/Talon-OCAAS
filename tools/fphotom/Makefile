# Makefile for fphotom

CLDFLAGS= 
CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math -I../../libastro  -I../../libwcs -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lwcs -lfits -lfs -lm

OBJS= fphotom.o

fphotom: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5:	install

install: fphotom
	rm -f $(TELHOME)/bin/fphotom
	cp fphotom $(TELHOME)/bin/fphotom

clobber:
	touch x.o
	rm -f *.o fphotom

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $
