# Makefile for several simple tools

CLDFLAGS = -ffast-math
CFLAGS = $(CLDFLAGS) -Wall -O2 -I../../libfits -I../../libmisc -I../../libastro -I../../libwcs
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lfits -lmisc -lastro -lm
DIR = $(TELHOME)/bin

ALL = degrad hrrad flipfits offsetfits jd lst mkfits raddeg radhr nisttime lookup

all: $(ALL)

pass5:	install

install: $(ALL)
	cd $(DIR); rm -f $(ALL)
	cp $(ALL) $(DIR)
	touch x.o
	rm *.o

degrad: degrad.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

flipfits: flipfits.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

hrrad: hrrad.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

jd: jd.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

lst: lst.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

mkfits: mkfits.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

raddeg: raddeg.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

radhr: radhr.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

nisttime: nisttime.o
	$(CC) $(LDFLAGS) $(MOTIFL) -o $@ $? $(MOTIFLIBS)

lookup: lookup.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

offsetfits: offsetfits.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS) -lfs -lwcs

clobber:
	touch x.o
	rm -f *.o $(ALL)
