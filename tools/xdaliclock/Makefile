CFLAGS = -Wall -O2 $(MOTIFI) -DSHAPE

OBJS = xdaliclock.o digital.o colors.o resources.o \
	visual.o hsv.o usleep.o

xdaliclock: $(OBJS)
	cc $(LDFLAGS) $(MOTIFL) -o $@ $(OBJS) $(MOTIFLIBS)

pass6:	install

install: xdaliclock
	rm -f $(TELHOME)/bin/xdaliclock
	cp xdaliclock $(TELHOME)/bin/xdaliclock

clobber:
	touch x.o
	rm -f *.o xdaliclock

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:47 $ $Revision: 1.1.1.1 $ $Name:  $
