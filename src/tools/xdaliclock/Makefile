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
