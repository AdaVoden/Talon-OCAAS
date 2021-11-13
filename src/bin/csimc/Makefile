CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libwcs -I../../libfs -I../../libastro -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lmisc -lfits -lwcs -lfs -lm

COBJS= csimc.o boot.o eintrio.o el.o

csimc: $(COBJS)
	$(CC) $(LDFLAGS) -o $@ $(COBJS) $(LIBS)

LOBJS= csimcio.o

csimcio: $(LOBJS)
	$(CC) $(LDFLAGS) -o $@ $(LOBJS) $(LIBS)

pass5: install

install: csimc csimcio
	rm -f $(TELHOME)/bin/csimc
	cp csimc $(TELHOME)/bin/csimc
	rm -f $(TELHOME)/bin/csimcio
	cp csimcio $(TELHOME)/bin/csimcio

eltest: el.c
	$(CC) $(CFLAGS) -DTESTEL -o eltest el.c
	rm -f el.o

clobber:
	touch x.o
	rm -fr *.o csimc csimcio eltest
