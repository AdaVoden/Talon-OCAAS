# Makefile for shm and telshow.
# Both are built from one source file, shm.c.

CLDFLAGS =
CFLAGS = $(CLDFLAGS) -O2 -ffast-math -Wall -I../../libastro -I../../libmisc
LDFLAGS = $(CLDFLAGS) -s -L$(TELHOME)/lib
XLDFLAGS = $(LDFLAGS) $(MOTIFL)
LIBS = -lm
XLIBS = $(MOTIFLIBS)
ASTROLIBS = -lastro -lmisc -lfits

all: shm telshow

pass6: shm
	rm -f $(TELHOME)/bin/shm
	cp shm $(TELHOME)/bin

pass5: telshow telshowl.sh
	rm -f $(TELHOME)/bin/telshow
	cp telshow $(TELHOME)/bin/telshow
	rm -f $(TELHOME)/bin/telshowl
	cp telshowl.sh $(TELHOME)/bin/telshowl

shm: shm.c
	$(CC) $(CFLAGS) $(MOTIFI) -DUSEX -c shm.c
	$(CC) $(XLDFLAGS) -o $@ shm.o $(ASTROLIBS) $(XLIBS) $(LIBS)
	rm shm.o

telshow: shm.c
	$(CC) ${CFLAGS} -c shm.c -o telshow.o
	$(CC) $(LDFLAGS) -o $@ telshow.o $(ASTROLIBS) $(LIBS)
	rm telshow.o

install: pass5 pass6

clobber:
	touch x.o
	rm -f *.o shm telshow

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:03 $ $Revision: 1.1.1.1 $ $Name:  $
