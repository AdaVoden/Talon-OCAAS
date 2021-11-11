# Makefile for mksch.

CLDFLAGS =
CFLAGS = $(CLDFLAGS) -Wall -ffast-math -O2 -I../../libmisc -I../../libXmisc $(MOTIFI)
LDFLAGS = -s $(CLDFLAGS) -L$(TELHOME)/lib $(MOTIFL)
LIBS = -lastro -lmisc -lfits -lXmisc -lm $(MOTIFLIBS)

OBJS =	mksch.o \
	msg.o \
	gui.o \
	sched.o \
	tips.o

mksch: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass6:	install

install: mksch
	rm -f $(TELHOME)/bin/mksch
	cp mksch $(TELHOME)/bin/mksch

clobber:
	touch x.o
	rm -f *.o mksch

