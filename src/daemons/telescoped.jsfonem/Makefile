# Makefile for telescoped.csi, Bisei ONEM version

CLDFLAGS = -O2 -ffast-math -Wall
CFLAGS = $(CLDFLAGS) -I../../libastro -I../../libmisc
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lm
SHELL = /bin/csh

OBJS =	telescoped.o \
	axes.o \
	csimc.o \
	dome.o \
	filter.o \
	focus.o \
	lights.o \
	tel.o \
	mountcor.o \
	powerfail.o \
	fifoio.o

telescoped.csi: $(OBJS)
	rm -f $@
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: telescoped.csi
	rm -f $(TELHOME)/bin/telescoped.csi
	cp telescoped.csi $(TELHOME)/bin/telescoped.csi

pass4jsf-onem:	install

clobber:
	touch x.o
	rm -f *.o telescoped.csi

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:59 $ $Revision: 1.1.1.1 $ $Name:  $
