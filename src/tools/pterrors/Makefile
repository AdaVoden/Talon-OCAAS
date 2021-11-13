IS =	-I../../libfits \
	-I../../libastro \
	-I../../libwcs \
	-I../../libmisc

CLDFLAGS=
CFLAGS= $(CLDFLAGS) -Wall -O2 -ffast-math $(IS)
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lwcs -lfs -lm

OBJSP= pterrors.o

pterrors: $(OBJSP)
	$(CC) $(LDFLAGS) -o $@ $(OBJSP) $(LIBS)

OBJSA= ptmeshadd.o

ptmeshadd: $(OBJSA)
	$(CC) $(LDFLAGS) -o $@ $(OBJSA) $(LIBS)

all:	ptmeshadd pterrors 

pass5:  install

install: ptmeshadd pterrors
	rm -f $(TELHOME)/bin/ptmeshadd
	cp ptmeshadd $(TELHOME)/bin/ptmeshadd
	rm -f $(TELHOME)/bin/pterrors
	cp pterrors $(TELHOME)/bin/pterrors

clobber:
	touch *.o
	rm -fr *.o ptmeshadd pterrors
