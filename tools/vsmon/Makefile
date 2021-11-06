CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math -I../../libastro  -I../../libwcs -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lwcs -lfs -lm

OBJS= vsmon.o

vsmon: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: vsmon
	rm -f $(TELHOME)/bin/vsmon
	cp vsmon $(TELHOME)/bin/vsmon

pass5: install

clobber:
	touch x.o
	rm -f *.o vsmon

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:46 $ $Revision: 1.1.1.1 $ $Name:  $
