# Makefile for telescoped.oms

CLDFLAGS = -O2 -ffast-math -Wall
CFLAGS = $(CLDFLAGS) -I../../libastro -I../../libmisc
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lm
SHELL = /bin/csh

OBJS =	telescoped.o \
	axes.o \
	dome.o \
	filter.o \
	focus.o \
	lights.o \
	tel.o \
	mountcor.o \
	powerfail.o \
	fifoio.o \
	pc39.o

telescoped.oms: $(OBJS)
	rm -f $@
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: telescoped.oms
	rm -f $(TELHOME)/bin/telescoped.oms
	cp telescoped.oms $(TELHOME)/bin/telescoped.oms

pass4:	install

clobber:
	touch x.o
	rm -f *.o telescoped.oms

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:58 $ $Revision: 1.1.1.1 $ $Name:  $
