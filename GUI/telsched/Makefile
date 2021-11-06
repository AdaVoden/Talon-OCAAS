# Makefile for telsched and chksch

CLDFLAGS =

IS =	\
	-I../../libmisc \
	-I../../libfits \
	-I../../libastro \
	-I../../libwcs	\
	-I../../libXmisc	\
	$(MOTIFI)
	
CFLAGS = $(CLDFLAGS) -Wall -O2 -ffast-math $(IS)
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib $(MOTIFL) -s
LIBS = -lfits -lmisc -lwcs -lastro -lXmisc -lfs $(MOTIFLIBS) -lm

TOBJS =				\
	camcal.o		\
	cat.o			\
	delsched.o		\
	edit.o			\
	obs.o			\
	obsdsp.o		\
	photstd.o		\
	pointing.o		\
	print.o			\
	readsls.o		\
	scangraphic.o		\
	sortscans.o		\
	telsched.o

telsched: $(TOBJS)
	$(CC) $(LDFLAGS) -o $@ $(TOBJS) $(LIBS)


COBJS =				\
	chksch.o		\
	obs.o

chksch: $(COBJS)
	$(CC) $(LDFLAGS) -o $@ $(COBJS) -lastro -lmisc -lfits -lm

all:	telsched chksch

pass5:	install_chksch

install_chksch: chksch
	rm -f $(TELHOME)/bin/chksch
	cp chksch $(TELHOME)/bin


pass6:	install_telsched

install_telsched: telsched
	rm -f $(TELHOME)/bin/telsched
	cp telsched $(TELHOME)/bin

install: install_chksch install_telsched

clobber:
	touch x.o
	rm -f *.o telsched chksch

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:04 $ $Revision: 1.1.1.1 $ $Name:  $
