CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -Wall
LDFLAGS= $(CLDFLAGS) -s
LIBS = -lm

OBJS= csi.o

csi: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: csi
	rm -f $(TELHOME)/bin/csi
	cp csi $(TELHOME)/bin/csi

clobber:
	touch x.o
	rm -fr *.o csi
