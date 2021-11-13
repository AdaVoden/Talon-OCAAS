CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libastro
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lm

OBJS= dynamics.o

dynamics: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: dynamics
	rm -f $(TELHOME)/bin/dynamics
	cp dynamics $(TELHOME)/bin/dynamics

clobber:
	touch x.o
	rm -fr *.o dynamics
