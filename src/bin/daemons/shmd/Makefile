CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libastro -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lmisc -lfits -lm

OBJS= shmd.o

shmd: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass4: install

install: shmd
	rm -f $(TELHOME)/bin/shmd
	cp shmd $(TELHOME)/bin/shmd

clobber:
	touch x.o
	rm -fr *.o shmd

