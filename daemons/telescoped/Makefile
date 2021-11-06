# Makefile for telescoped

CLDFLAGS = -O2 -ffast-math -Wall
CFLAGS = $(CLDFLAGS) -I../../libastro -I../../libmisc
LDFLAGS = -s $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lm
SHELL = /bin/csh

OBJS =	telescoped.o

telescoped: $(OBJS)
	rm -f $@
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: telescoped
	rm -f $(TELHOME)/bin/telescoped
	cp telescoped $(TELHOME)/bin/telescoped

pass4:	install

clobber:
	touch x.o
	rm -f *.o telescoped

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $
