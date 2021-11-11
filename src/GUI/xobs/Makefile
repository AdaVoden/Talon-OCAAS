# Makefile for xobs.

IS =	-I../../libastro \
	-I../../libmisc \
	-I../../libfits \
	-I../../libfs \
	-I../../libXmisc \
	-I../../libwcs

CLDFLAGS = 
CFLAGS = $(CLDFLAGS) -Wall -ffast-math -O2 $(IS) $(MOTIFI)
LDFLAGS = -s -L$(TELHOME)/lib $(CLDFLAGS) $(MOTIFL)
LIBS = -lastro -lfits -lmisc -lwcs -lfs -lXmisc -lm $(MOTIFLIBS)

OBJS =	xobs.o \
	autofocus.o \
	batch.o \
	calaxes.o \
	config.o \
	control.o \
	dome.o \
	fifos.o \
	gui.o \
	paddle.o \
	query.o \
	scope.o \
	skymap.o \
	sound.o \
	telrun.o \
	tips.o \
	update.o \
	xephem.o

xobs: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass3:
	rm -f buildcfg.h
	touch buildcfg.h

pass3jsf-halfm:
	echo "#define WINDSCREEN 0" >> buildcfg.h

pass3jsf-onem:
	echo "#define COVER 1" >> buildcfg.h

pass6:	install

install: xobs
	rm -f $(TELHOME)/bin/xobs
	cp xobs $(TELHOME)/bin/xobs

clobber:
	touch x.o
	rm -f *.o xobs

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:40 $ $Revision: 1.1.1.1 $ $Name:  $
