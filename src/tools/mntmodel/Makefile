CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libastro -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lfits -lmisc -lm

OBJS= mntmodel.o

mntmodel: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: mntmodel
	rm -f $(TELHOME)/bin/mntmodel
	cp mntmodel $(TELHOME)/bin/mntmodel

clobber:
	touch x.o
	rm -fr *.o mntmodel
