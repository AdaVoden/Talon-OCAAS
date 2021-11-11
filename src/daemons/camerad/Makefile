# Makefile for camerad

CLDFLAGS = -O2 -Wall
CFLAGS = $(CLDFLAGS) -I../../libfits -I../../libastro -I../../libmisc
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lm

OBJS = camerad.o

camerad: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass4:	install

install: camerad
	rm -f $(TELHOME)/bin/camerad
	cp camerad $(TELHOME)/bin/camerad

clobber:
	touch x.o
	rm -f *.o camerad

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $
