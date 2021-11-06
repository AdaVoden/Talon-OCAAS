CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libastro -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lmisc -lfits -lm

OBJS= fio.o

fio: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: fio
	rm -f $(TELHOME)/bin/fio
	cp fio $(TELHOME)/bin/fio

clobber:
	touch x.o
	rm -fr *.o fio

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:33 $ $Revision: 1.1.1.1 $ $Name:  $
